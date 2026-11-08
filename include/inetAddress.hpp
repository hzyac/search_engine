// inetaddress.hpp
#pragma once

#include <arpa/inet.h>
#include <string>

using std::string;

class InetAddress {
public:
        // 绑定ip和端口号
        InetAddress(const string &ip, unsigned short port);
        // 绑定ip和端口号
        InetAddress(const struct sockaddr_in &addr);
        ~InetAddress();
        // 获取ip地址
        string ip() const;
        // 获取端口号
        unsigned short port() const;
        // 获取sockaddr_in结构体指针
        const struct sockaddr_in *getinetAddressPtr() const;
private:
    struct sockaddr_in _addr;
};
