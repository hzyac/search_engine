// EchoServer.cpp
#include "../include/echoServer.hpp"
#include "../include/tcpServer.hpp"
#include "../include/myTask.hpp"

#include <iostream>

using std::cout;
using std::endl;

EchoServer::EchoServer(size_t threadNum, size_t queSize, const string &ip, unsigned short port)
: _pool(threadNum, queSize)
, _server(ip, port)
{

}

EchoServer::~EchoServer()
{

}

// 启动服务器
void EchoServer::start()
{
    _pool.start();

    // 注册所有的回调函数
    using namespace std::placeholders;
    _server.setAllCallback(std::bind(&EchoServer::onNewConnection, this, _1),
                           std::bind(&EchoServer::onMessage, this, _1),
                           std::bind(&EchoServer::onClose, this, _1));

    _server.start();
}

// 关闭服务器
void EchoServer::stop()
{
    _pool.stop();
    _server.stop();
}

// 注册三个回调函数
void EchoServer::onNewConnection(const tcpConnectionPtr &con)
{
    cout << con->toString() << " has connected!!!" << endl;
}

void EchoServer::onMessage(const tcpConnectionPtr &con)
{
    // 接收用户的信息
    string msg = con->receive();
    if(msg.empty())
    {
        cout << ">>recv empty msg, skip" << endl;
        return;
    }
    cout << ">>recv msg from client: " << msg << endl;

    // 对用户发送过来的信息进行处理
    MyTask MyTask(msg, 
                  con, 
                  "../data/dict.dat", 
                  "../data/dictIndex.dat",
                  "../data/newripepage.dat",
                  "../data/newoffset.dat",
                  "../data/invertIndex.dat");
    // 扔到线程池中去处理, 这里就当于是线程池传递信号
    using std::placeholders::_1;
    _pool.addTask(std::bind(&MyTask::process, MyTask, _1));
}

void EchoServer::onClose(const tcpConnectionPtr &con)
{
    cout << con->toString() << " has closed!!!" << endl;
}