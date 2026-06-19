// TcpConnection.hpp
#ifndef SEARCH_ENGINE_TCP_CONNECTION_HPP
#define SEARCH_ENGINE_TCP_CONNECTION_HPP

#include "socketIO.hpp"
#include "socket.hpp"
#include "inetAddress.hpp"
#include <memory>
#include <functional>

using std::function;
using std::shared_ptr;

class EventLoop;

class TcpConnection
:public std::enable_shared_from_this<TcpConnection>
{
public:
    using tcpConnectionPtr = shared_ptr<TcpConnection>;
    using tcpConnectionCallback = function<void(const tcpConnectionPtr&)>;

public:
    explicit TcpConnection(int fd, EventLoop *loop);
    ~TcpConnection();

    // 发送信息
    void send(const string &msg);
    // 让eventLoop将任务存储起来, 同让其时去处理这个任务
    void sendInLoop(const string &msg);
    // 接收信息
    string receive();

    // 查看链接是否断开
    bool isClose();

    // 为了方便调试的函数
    string toString() const;

public:
    // 三个回调函数的注册
    void setNewConnectionCallback(const tcpConnectionCallback &cb);
    void setMessageCallback(const tcpConnectionCallback &cb);
    void setCloseCallback(const tcpConnectionCallback &cb);

    // 三个回调函数的运行
    void handleNewConnectionCallback();
    void handleMessageCallback();
    void handleCloseCallback();

private:
    // 获取本端地址和对端地址
    InetAddress getLocalAddr();
    InetAddress getPeerAddr(); 

private:
    // 让tcpConnection知道eventLoop的存在
    EventLoop *_loop; 
    SocketIO _sockIO;

    // 为了调试而加入的三个数据成员
    Socket _sock;
    InetAddress _localAddress;
    InetAddress _peerAddress;

private:
    // 建立连接
    tcpConnectionCallback _onNewConnection;
    // 消息到达
    tcpConnectionCallback _onMessage;
    // 断开连接
    tcpConnectionCallback _onClose;
};

#endif
