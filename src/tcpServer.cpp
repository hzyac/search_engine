// TcpServer.cpp
#include "../include/tcpServer.hpp"
#include <iostream>
#include <cstdio>

using std::cout;
using std::endl;

TcpServer::TcpServer(const string &ip, unsigned short port)
: _acceptor(ip, port)
, _loop(_acceptor)
{
    cout << "TcpServer() 构造, ip: " << ip << ", port: " << port << endl;
}

TcpServer::~TcpServer()
{
    cout << "~TcpServer()" << endl;
}

// 服务器的启动
void TcpServer::start()
{
    cout << "TcpServer::start() 启动服务器" << endl;
    _acceptor.ready();
    _loop.loop();
}

// 服务器停止
void TcpServer::stop()
{
    cout << "TcpServer::stop() 停止服务器" << endl;
    _loop.unloop();
}

// 三个回调函数的注册
void TcpServer::setAllCallback(tcpConnectionCallback &&cb1, tcpConnectionCallback &&cb2, tcpConnectionCallback &&cb3)
{
    _loop.setNewConnectionCallback(std::move(cb1));
    _loop.setMessageCallback(std::move(cb2));
    _loop.setCloseCallback(std::move(cb3));
}