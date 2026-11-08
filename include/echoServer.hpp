// EchoServer.hpp
#pragma once

#include "eventLoop.hpp"
#include "threadPool.hpp"
#include "tcpServer.hpp"

class EchoServer {
public:
    using tcpConnectionPtr = shared_ptr<TcpConnection>;
public:
    EchoServer(size_t threadNum, size_t queSize, const string &ip, unsigned short port);
    ~EchoServer();

    // 启动服务器
    void start();
    // 关闭服务器
    void stop();

    // 注册三个回调函数
    void onNewConnection(const tcpConnectionPtr &con);
    void onMessage(const tcpConnectionPtr &con);
    void onClose(const tcpConnectionPtr &con);

private:
    ThreadPool _pool;
    TcpServer _server;

};
