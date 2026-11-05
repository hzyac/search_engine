// EchoServer.hpp
#ifndef SEARCH_ENGINE_ECHO_SERVER_HPP
#define SEARCH_ENGINE_ECHO_SERVER_HPP

#include "eventLoop.hpp"
#include "runtimeConfig.hpp"
#include <memory>

class SplitTool;
class IndexManager;
#include "threadPool.hpp"
#include "tcpServer.hpp"

class EchoServer
{
public:
    using tcpConnectionPtr = std::shared_ptr<TcpConnection>;

public:
    explicit EchoServer(const RuntimeConfig &config);
    ~EchoServer();

    // 启动服务器
    void start();

    // 关闭服务器
    void stop();

    // 注册三个回调函数
    void onNewConnection(const tcpConnectionPtr &con);
    void onMessage(const tcpConnectionPtr &con);
    void onClose(const tcpConnectionPtr &con);

private:
    RuntimeConfig _config;
    std::shared_ptr<IndexManager> _indexManager;
    std::shared_ptr<SplitTool> _splitTool;
    ThreadPool _pool;
    TcpServer _server;
};

#endif
