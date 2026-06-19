// TcpConnection.cpp
#include "../include/tcpConnection.hpp"
#include "../include/eventLoop.hpp"
#include <iostream>
#include <netinet/in.h>
#include <sstream>
#include <sys/socket.h>

using std::ostringstream;
using std::cerr;
using std::cout;
using std::endl;

TcpConnection::TcpConnection(int fd, EventLoop *loop)
: _loop(loop)
, _sockIO(fd)
, _sock(fd)
, _localAddress(getLocalAddr())
, _peerAddress(getPeerAddr())
{
    cout << "TcpConnection() 构造, fd: " << fd
         << ", local: " << _localAddress.ip() << ":" << _localAddress.port()
         << ", peer: " << _peerAddress.ip() << ":" << _peerAddress.port() << endl;
}

TcpConnection::~TcpConnection()
{
    cout << "~TcpConnection() 析构, fd: " << _sock.fd() << endl;
}

// 发送信息
void TcpConnection::send(const string &msg)
{
    int ret = _sockIO.writen(msg.c_str(), msg.size());
    cout << "TcpConnection::send() fd: " << _sock.fd() << ", 发送 " << ret << " 字节" << endl;
}

// 让eventLoop将任务存储起来, 同时让其去处理这个任务
void TcpConnection::sendInLoop(const string &msg)
{
    if(_loop)
    {
        cout << "TcpConnection::sendInLoop() fd: " << _sock.fd() << ", 投递到 EventLoop" << endl;
        _loop->runInLoop(bind(&TcpConnection::send, this, msg));
    }
}

// 接收信息
string TcpConnection::receive()
{
    char buf[65535] = {0};
    int ret = _sockIO.readTrain(buf, sizeof(buf));
    cout << "TcpConnection::receive() fd: " << _sock.fd() << ", 接收 " << ret << " 字节" << endl;
    if(ret <= 0)
    {
        return string();
    }
    return string(buf, ret);
}

// 查看链接是否断开
bool TcpConnection::isClose()
{
   char buf[10] = {0};
   int ret = ::recv(_sock.fd(), buf, sizeof(buf), MSG_PEEK);
   if(ret <= 0)
   {
       cout << "TcpConnection::isClose() fd: " << _sock.fd() << " 连接已断开 (ret=" << ret << ")" << endl;
   }
   return ret <= 0;
}

// 为了方便调试的函数
string TcpConnection::toString() const
{
    ostringstream oss;
    oss << _localAddress.ip() << ":"
        << _localAddress.port() << "---->"
        << _peerAddress.ip() << ":"
        << _peerAddress.port();

    return oss.str();
}

// 三个回调函数的注册
void TcpConnection::setNewConnectionCallback(const tcpConnectionCallback &cb)
{
    _onNewConnection = cb;  
}

void TcpConnection::setMessageCallback(const tcpConnectionCallback &cb)
{
    _onMessage = cb;
}

void TcpConnection::setCloseCallback(const tcpConnectionCallback &cb)
{
    _onClose = cb;
}

// 三个回调函数的运行
void TcpConnection::handleNewConnectionCallback()
{
    if(_onNewConnection)
    {
        _onNewConnection(shared_from_this());
    }
    else
    {
        cerr << "nullptr == _onNewConnection" << endl;
        return;
    }
}

void TcpConnection::handleMessageCallback()
{
    if(_onMessage)
    {
        _onMessage(shared_from_this());
    }
    else
    {
        cerr << "nullptr == _onMessage" << endl;
        return;
    }
}

void TcpConnection::handleCloseCallback()
{
    if(_onClose)
    {
        _onClose(shared_from_this());
    }
    else
    {
        cerr << "nullptr == _onClose" << endl;
        return;
    }
}

// 获取本端地址
InetAddress TcpConnection::getLocalAddr()
{
    struct sockaddr_in addr;
    socklen_t len = sizeof(sockaddr_in);
    // 获取本端ip地址
    int ret = ::getsockname(_sock.fd(), (struct sockaddr*)&addr, &len);
    if(-1 == ret)
    {
        cerr << "getsockname" << endl;
    }
    return InetAddress(addr);
}

// 获取对端地址
InetAddress TcpConnection::getPeerAddr()
{
    struct sockaddr_in addr;
    socklen_t len = sizeof(sockaddr_in);
    // 获取对端ip地址
    int ret = ::getpeername(_sock.fd(), (struct sockaddr*)&addr, &len);
    if(-1 == ret)
    {
        cerr << "getpeername" << endl;
    }
    return InetAddress(addr);
}
