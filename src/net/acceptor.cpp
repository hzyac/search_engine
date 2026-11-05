// Acceptor.cpp
#include "acceptor.hpp"
#include "socket.hpp"

#include <iostream>
#include <system_error>
#include <cerrno>

// 绑定地址和端口
Acceptor::Acceptor(const std::string &ip, unsigned short port)
: _sock()
, _addr(ip, port)
{
}

// 析构函数, 暂时用不到, 因为地址对象和套接字对象都利用RAII思想在析构的时候可以自动释放资源
Acceptor::~Acceptor()
{
    std::cout << "~Acceptor()" << std::endl;
}

// 启动服务器
void Acceptor::ready()
{
    setReuseAddr();

    // A second server must fail rather than share this endpoint.
    bind();
    listen();
    std::cout << "Acceptor::ready() 完成, 服务器准备就绪" << std::endl;
}

// 获取地址对象的socket_fd
int Acceptor::fd()
{
    return _sock.fd();
}

// 设置地址复用
void Acceptor::setReuseAddr()
{
    int opt = 1;
    int ret = setsockopt(_sock.fd(), SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    if(ret == -1)
    {
        std::cerr << "setsockopt SO_REUSEADDR failed" << std::endl;
        return;
    }
}

// 设置端口复用
void Acceptor::setReusePort()
{
    int opt = 1;
    int ret = setsockopt(_sock.fd(), SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt));
    if(ret == -1)
    {
        std::cerr << "setsockopt SO_REUSEPORT failed" << std::endl;
        return;
    }
}

// 绑定地址和端口
void Acceptor::bind()
{
    int ret = ::bind(_sock.fd(), (const struct sockaddr *)_addr.getinetAddressPtr(),
                     sizeof(*_addr.getinetAddressPtr()));
    if(-1 == ret)
    {
        throw std::system_error(errno, std::generic_category(), "bind");
    }

    std::cout << "Acceptor::bind() 成功, ip: " << _addr.ip() << ", port: " << _addr.port()
              << std::endl;
}

// 监听
void Acceptor::listen()
{
    int ret = ::listen(_sock.fd(), 128);
    if(-1 == ret)
    {
        throw std::system_error(errno, std::generic_category(), "listen");
    }

    std::cout << "Acceptor::listen() 成功, fd: " << _sock.fd() << ", 开始监听" << std::endl;
}

// 接收连接, 返回连接套接字的文件描述符
int Acceptor::accept()
{
    int connfd = ::accept(_sock.fd(), nullptr, nullptr);
    if(-1 == connfd)
    {
        std::cerr << "accept" << std::endl;
        return -1;
    }

    std::cout << "Acceptor::accept() 新连接, connfd: " << connfd << std::endl;
    return connfd;
}
