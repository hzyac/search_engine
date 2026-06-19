// Socket.cpp
#include "../include/socket.hpp"
#include <iostream>
#include <sys/socket.h>
#include <unistd.h>

using std::cerr;
using std::endl;

// 创建套接字
Socket::Socket()
{
    _fd = ::socket(AF_INET, SOCK_STREAM, 0);
    if(_fd == -1)
    {
        cerr << "Socket" << endl;
        return;
    }
}

Socket::Socket(int fd)
: _fd(fd)
{

}

// 关闭套接字
Socket::~Socket()
{
    close(_fd);
}

int Socket::fd() const
{
    return _fd;
}