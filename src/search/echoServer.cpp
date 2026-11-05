// EchoServer.cpp
#include "echoServer.hpp"
#include "indexManager.hpp"
#include "tcpServer.hpp"
#include "myTask.hpp"
#include "splitToolCppJieba.hpp"

#include <iostream>

EchoServer::EchoServer(const RuntimeConfig &config)
: _config(config)
, _indexManager(std::make_shared<IndexManager>(config))
, _splitTool(std::make_shared<SplitToolCppJieba>(config))
, _pool(config.threads, config.queueSize)
, _server(config.host, config.port)
{
}

EchoServer::~EchoServer()
{
    _pool.stop();
}

// 启动服务器
void EchoServer::start()
{
    _server.prepare();
    _pool.start();

    // 注册所有的回调函数
    _server.setAllCallback(std::bind(&EchoServer::onNewConnection, this, std::placeholders::_1),
                           std::bind(&EchoServer::onMessage, this, std::placeholders::_1),
                           std::bind(&EchoServer::onClose, this, std::placeholders::_1));

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
    std::cout << con->toString() << " has connected!!!" << std::endl;
}

void EchoServer::onMessage(const tcpConnectionPtr &con)
{
    // 接收用户的信息
    std::string msg = con->receive();
    if(msg.empty())
    {
        std::cout << ">>recv empty msg, skip" << std::endl;
        return;
    }

    std::cout << ">>recv msg from client: " << msg << std::endl;

    // 对用户发送过来的信息进行处理
    MyTask MyTask(msg, con, _splitTool, _indexManager);

    // 扔到线程池中去处理, 这里就当于是线程池传递信号
    _pool.addTask(std::bind(&MyTask::process, MyTask, std::placeholders::_1));
}

void EchoServer::onClose(const tcpConnectionPtr &con)
{
    std::cout << con->toString() << " has closed!!!" << std::endl;
}
