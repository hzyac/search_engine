// InetAddress.cpp
#include "inetAddress.hpp"
#include <arpa/inet.h>
#include <cstring>
#include <netinet/in.h>

// 绑定ip和端口号
InetAddress::InetAddress(const std::string &ip, unsigned short port)
{
    ::bzero(&_addr, sizeof(struct sockaddr_in));

    _addr.sin_family = AF_INET;
    _addr.sin_addr.s_addr = inet_addr(ip.c_str());
    _addr.sin_port = htons(port);
}

// 绑定ip和端口号
InetAddress::InetAddress(const struct sockaddr_in &addr)
: _addr(addr)
{
}

InetAddress::~InetAddress()
{
}

// 获取ip地址
std::string InetAddress::ip() const
{
    return std::string(inet_ntoa(_addr.sin_addr));
}

// 获取端口号
unsigned short InetAddress::port() const
{
    return ntohs(_addr.sin_port);
}

// 获取sockaddr_in结构体指针
const struct sockaddr_in *InetAddress::getinetAddressPtr() const
{
    return &_addr;
}
