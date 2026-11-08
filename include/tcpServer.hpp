// TcpServer.hpp
#pragma once

#include "acceptor.hpp"
#include "eventLoop.hpp"
#include <string>

using std::string;

class TcpConnection;

class TcpServer {
public:
    using tcpConnectionPtr = shared_ptr<TcpConnection>;
    using tcpConnectionCallback = function<void(const tcpConnectionPtr&)>;

public:
    TcpServer(const string &ip, unsigned short port);
    ~TcpServer();

    // 服务器的启动
    void start();
    // 服务器停止
    void stop();

    // 三个回调函数的注册
    void setAllCallback(tcpConnectionCallback &&cb1, tcpConnectionCallback &&cb2, tcpConnectionCallback &&cb3);
private:
    Acceptor _acceptor;
    EventLoop _loop;
};
