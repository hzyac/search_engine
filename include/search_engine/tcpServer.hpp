// TcpServer.hpp
#ifndef SEARCH_ENGINE_TCP_SERVER_HPP
#define SEARCH_ENGINE_TCP_SERVER_HPP

#include "acceptor.hpp"
#include "eventLoop.hpp"
#include <string>

class TcpConnection;

class TcpServer
{
public:
    using tcpConnectionPtr = std::shared_ptr<TcpConnection>;
    using tcpConnectionCallback = std::function<void(const tcpConnectionPtr &)>;

public:
    TcpServer(const std::string &ip, unsigned short port);
    ~TcpServer();

    // 服务器的启动
    void prepare();
    void start();

    // 服务器停止
    void stop();

    // 三个回调函数的注册
    void setAllCallback(tcpConnectionCallback &&cb1, tcpConnectionCallback &&cb2,
                        tcpConnectionCallback &&cb3);

private:
    Acceptor _acceptor;
    EventLoop _loop;
};

#endif
