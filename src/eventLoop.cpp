// EventLoop.cpp
#include "../include/tcpConnection.hpp"
#include "../include/eventLoop.hpp"
#include "../include/acceptor.hpp"
#include <cerrno>
#include <iostream>
#include <mutex>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <unistd.h>

using std::cerr;
using std::cout;
using std::endl;
using std::lock_guard;

EventLoop::EventLoop(Acceptor &Acceptor)
: _epfd(createEpollFd())
, _evList(1024)
, _isLooping(false)
, _acceptor(Acceptor)
, _evtfd(createEventFd())
, _mutex()
{
    cout << "EventLoop() 构造, epfd: " << _epfd << ", evtfd: " << _evtfd << endl;
    // 将listenfd放在红黑树上监听
    int listenfd = _acceptor.fd();
    addEpollReadFd(listenfd);
    cout << "EventLoop 添加 listenfd(" << listenfd << ") 到 epoll" << endl;

    // 将通信文件描述符_evtfd放在文件描述符上监听
    addEpollReadFd(_evtfd);
    cout << "EventLoop 添加 evtfd(" << _evtfd << ") 到 epoll" << endl;
}

EventLoop::~EventLoop()
{
    cout << "~EventLoop() 关闭 epfd: " << _epfd << ", evtfd: " << _evtfd << endl;
    close(_epfd);
    close(_evtfd);
}

// 循环监听
void EventLoop::loop()
{
    _isLooping = true;
    cout << "EventLoop::loop() 事件循环开始" << endl;
    while(_isLooping)
    {
        waitEpollFd();
    }
    cout << "EventLoop::loop() 事件循环结束" << endl;
}

// 退出循环监听
void EventLoop::unloop()
{
    cout << "EventLoop::unloop() 退出事件循环" << endl;
    _isLooping = false;
}

// 封装epoll_wait
void EventLoop::waitEpollFd()
{
    // 防止信号中断
    int nready = 0;
    do 
    {
        nready = ::epoll_wait(_epfd, &*_evList.begin(), _evList.size(), 30000);
    }while(-1 == nready && errno == EINTR);

    if(-1 == nready)
    {
        cerr << "-1 == nready" << endl;
        return;
    }
    else if(0 == nready)
    {
        cout << ">>epoll_wait timeout!!!" << endl;
    }
    else
    {
        // 防止文件描述符超过1024, 进行动态扩容
        if(nready == (int)_evList.size())
        {
            _evList.resize(nready * 2);
        }

        for(int idx = 0; idx < nready; ++idx)
        {
            int fd = _evList[idx].data.fd;
            int listenfd = _acceptor.fd();

            if(fd == listenfd)
            {
                // 处理新的请求
                handleNewConnection(); 
            }
            else if(fd == _evtfd)
            {
                // 被通知任务处理好了, 现在需要把这个通信接收一下
                handleRead();
                // 然后处理一下处理好的任务
                doPendingFunctors();
            }
            else 
            {
                // 处理老的连接
                handleMessage(fd);
            }
        }
    }
}

// 处理新的连接
void EventLoop::handleNewConnection()
{
    int connfd = _acceptor.accept();
    if(-1 == connfd)
    {
        cerr << "handleNewConnection" << endl;
        return;
    }
    cout << "EventLoop::handleNewConnection() 新客户端连接, connfd: " << connfd << endl;
    
    // 加入红黑树上监听
    addEpollReadFd(connfd);
    cout << "EventLoop 添加 connfd(" << connfd << ") 到 epoll" << endl;

    // 三次握手成功, 可以建立tcpConnection
    tcpConnectionPtr con(new TcpConnection(connfd, this));

    // 传递三个回调函数
    // 建立连接
    con->setNewConnectionCallback(_onNewConnection);
    // 消息到达
    con->setMessageCallback(_onMessage);
    // 断开连接
    con->setCloseCallback(_onClose);
 
    // 存储老的连接
    _conns[connfd] = con;
    cout << "EventLoop 存储 connfd(" << connfd << ") 到 _conns, 当前连接数: " << _conns.size() << endl;

    // 可以执行建立连接的事件了
    con->handleNewConnectionCallback();
}

// 处理老的连接上的数据
void EventLoop::handleMessage(int fd)
{
    cout << "EventLoop::handleMessage() fd: " << fd << " 有数据到达" << endl;
    map<int, tcpConnectionPtr>::iterator it = _conns.find(fd);
    if(it != _conns.end())
    {
        // 判断连接是否断开
        bool flag = it->second->isClose();
        // 已经断开
        if(flag)
        {
            cout << "EventLoop 检测到 connfd(" << fd << ") 已断开" << endl;
            // 处理断开的事件
            // 回调断开连接函数
            it->second->handleCloseCallback(); 
            // 连接断开需要从红黑树上摘除掉
            delEpollReadFd(fd);
            cout << "EventLoop 从 epoll 移除 connfd(" << fd << ")" << endl;
            // 还需要将文件描述符键值对从map中删除
            _conns.erase(it);
            cout << "EventLoop 从 _conns 移除 connfd(" << fd << "), 当前连接数: " << _conns.size() << endl;
        }
        else
        {
            // 回调消息到达函数
            it->second->handleMessageCallback();
        }
    }
    else
    {
        cout << "连接是不存在的" << endl;
        return;
    }
}

int EventLoop::createEpollFd()
{
    int fd = ::epoll_create1(0);
    if(fd == -1)
    {
        cerr << "epoll_create1 failed" << endl;
        return -1;
    }
    return fd;
}

// 将文件描述符放在红黑树上监听
void EventLoop::addEpollReadFd(int fd)
{
    struct epoll_event event;
    event.events = EPOLLIN;
    event.data.fd = fd;

    int ret = ::epoll_ctl(_epfd, EPOLL_CTL_ADD, fd, &event);
    if(ret == -1)
    {
        cerr << "addEpollReadFd" << endl;
        return;
    }
}

// 将文件描述符从红黑树上取消监听
void EventLoop::delEpollReadFd(int fd)
{
    struct epoll_event event;
    event.events = EPOLLIN;
    event.data.fd = fd;

    int ret = ::epoll_ctl(_epfd, EPOLL_CTL_DEL, fd, &event);
    if(ret == -1)
    {
        cerr << "delEpollReadFd" << endl;
        return;
    }
}

// 三个回调函数的注册
void EventLoop::setNewConnectionCallback(const tcpConnectionCallback &&cb)
{
    _onNewConnection = std::move(cb);
}

void EventLoop::setMessageCallback(const tcpConnectionCallback &&cb)
{
    _onMessage = std::move(cb);
}

void EventLoop::setCloseCallback(const tcpConnectionCallback &&cb)
{
    _onClose = std::move(cb);
}

// 创建用于通信的文件描述符
int EventLoop::createEventFd()
{
    // 初始值是0, 不设置标志位
    int efd = eventfd(0, 0);
    if(efd < 0)
    {
        cerr << "eventfd" << endl;
        return -1;
    }
    return efd;
}

// 读通信文件描述符
void EventLoop::handleRead()
{
    uint64_t one = 1;
    ssize_t ret = read(_evtfd, &one, sizeof(uint64_t));
    if(ret != sizeof(uint64_t))
    {
        cerr << "handleRead" << endl;
        return;
    }
    cout << "EventLoop::handleRead() 收到 eventfd 通知" << endl;
}

// 写通信文件描述符
void EventLoop::wakeup()
{
    uint64_t one = 1;
    ssize_t ret = write(_evtfd, &one, sizeof(uint64_t));
    if(ret != sizeof(uint64_t))
    {
        cerr << "wakeup" << endl;
        return;
    }
    cout << "EventLoop::wakeup() 发送 eventfd 通知" << endl;
}

// eventLoop遍历完成处理的任务(这里是进行发送信息)
void EventLoop::doPendingFunctors()
{
    vector<Functors>tmp;
    {
        lock_guard<mutex> lg(_mutex);
        tmp.swap(_pendings);
    }

    cout << "EventLoop::doPendingFunctors() 处理 " << tmp.size() << " 个待办任务" << endl;
    for(Functors &cb : tmp)
    {
        cb(); // 执行任务
    }
}

// 通过线程池调用这个函数存储处理好的任务, 并通过wakeup通知eventLoop
void EventLoop::runInLoop(Functors &&cb)
{
    {
        lock_guard<mutex>lg(_mutex);
        _pendings.push_back(std::move(cb));
    }
    // 任务处理完了, 现在要执行cb这个函数将信息发送出去, 调用cb函数教给其他人来做
    wakeup();
}