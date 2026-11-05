// eventloop.hpp
#ifndef SEARCH_ENGINE_EVENT_LOOP_HPP
#define SEARCH_ENGINE_EVENT_LOOP_HPP

#include <memory>
#include <functional>
#include <vector>
#include <map>
#include <mutex>

class TcpConnection;
class Acceptor;

class EventLoop
{
public:
    using tcpConnectionPtr = std::shared_ptr<TcpConnection>;
    using tcpConnectionCallback = std::function<void(const tcpConnectionPtr &)>;
    using Functors = std::function<void()>;

public:
    EventLoop(Acceptor &Acceptor);
    ~EventLoop();

    // 循环监听
    void loop();

    // 退出循环监听
    void unloop();

    // 封装epoll_wait
    void waitEpollFd();

    // 处理新的连接
    void handleNewConnection();

    // 处理老的连接上的数据
    void handleMessage(int fd);

    int createEpollFd();

    // 将文件描述符放在红黑树上监听
    void addEpollReadFd(int fd);

    // 将文件描述符从红黑树上取消监听
    void delEpollReadFd(int fd);

public:
    // 三个回调函数的注册
    void setNewConnectionCallback(const tcpConnectionCallback &&cb);
    void setMessageCallback(const tcpConnectionCallback &&cb);
    void setCloseCallback(const tcpConnectionCallback &&cb);

public:
    // 创建用于通信的文件描述符
    int createEventFd();

    // 读通信文件描述符
    void handleRead();

    // 写通信文件描述符
    void wakeup();

    // eventLoop遍历完成处理的任务(这里是进行发送信息)
    void doPendingFunctors();

    // 通过线程池调用这个函数存储处理好的任务, 并通过wakeup通知eventLoop
    void runInLoop(Functors &&cb);

private:
    // epoll_create创建的文件描述符
    int _epfd;

    // 存放就绪的文件描述符
    std::vector<struct epoll_event> _evList;

    // 标识循环是否在运行
    bool _isLooping;

    //目的是调用accept函数
    Acceptor &_acceptor;

    // 存放文件描述符与tcpConnection的键值对
    std::map<int, tcpConnectionPtr> _conns;

    // 建立连接
    tcpConnectionCallback _onNewConnection;

    // 消息到达
    tcpConnectionCallback _onMessage;

    // 断开连接
    tcpConnectionCallback _onClose;

    // 用于通信的文件描述符
    int _evtfd;

    // 存放多个互斥的任务
    std::vector<Functors> _pendings;

    // 对_pending互斥访问
    std::mutex _mutex;
};

#endif
