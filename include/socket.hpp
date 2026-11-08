// Socket.hpp
#pragma once

#include "nonCopyable.hpp"

// 封装获取套接字, 利用RAII思想在析构的时候可以关闭fd
class Socket
:private NonCopyable
{
public:
    // 创建套接字
    Socket();
    Socket(int fd);
    // 关闭套接字
    ~Socket();
    // 获取文件描述符 
    int fd() const; 
    // 关闭写端
    void shutDownWrite();

private:
    int _fd;
};
