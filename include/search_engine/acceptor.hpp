// Acceptor.hpp
#ifndef SEARCH_ENGINE_ACCEPTOR_HPP
#define SEARCH_ENGINE_ACCEPTOR_HPP

#include "socket.hpp"
#include "inetAddress.hpp"

class Acceptor
{
public:
    // 绑定地址和端口
    Acceptor(const std::string &ip, unsigned short port);

    // 析构函数, 暂时用不到, 因为地址对象和套接字对象都利用RAII思想在析构的时候可以自动释放资源
    ~Acceptor();

    // 启动服务器
    void ready();

    // 接收连接, 返回连接套接字的文件描述符
    int accept();

    // 获取地址对象的socket_fd
    int fd();

private:
    // 设置地址复用
    void setReuseAddr();

    // 设置端口复用
    void setReusePort();

    // 绑定地址和端口
    void bind();

    // 监听
    void listen();

private:
    // 套接字对象, 封装socket函数创建套接字, 利用RAII思想在析构的时候可以关闭fd
    Socket _sock;

    // 地址对象, 封装配置地址和端口
    InetAddress _addr;
};

#endif
