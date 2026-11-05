// TcpServer.cpp
#include "tcpServer.hpp"
#include <iostream>
#include <cstdio>

TcpServer::TcpServer(const std::string &ip, unsigned short port)
: _acceptor(ip, port)
, _loop(_acceptor)
{
    std::cout << "TcpServer() 构造, ip: " << ip << ", port: " << port << std::endl;
}

TcpServer::~TcpServer()
{
    std::cout << "~TcpServer()" << std::endl;
}

// 服务器的启动
void TcpServer::start()
{
    std::cout << "TcpServer::start() 启动服务器" << std::endl;
    _loop.loop();
}

void TcpServer::prepare()
{
    _acceptor.ready();
}

// 服务器停止
void TcpServer::stop()
{
    std::cout << "TcpServer::stop() 停止服务器" << std::endl;
    _loop.unloop();
}

// 三个回调函数的注册
void TcpServer::setAllCallback(tcpConnectionCallback &&cb1, tcpConnectionCallback &&cb2,
                               tcpConnectionCallback &&cb3)
{
    _loop.setNewConnectionCallback(std::move(cb1));
    _loop.setMessageCallback(std::move(cb2));
    _loop.setCloseCallback(std::move(cb3));
}
