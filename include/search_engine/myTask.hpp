// MyTask.hpp
#ifndef SEARCH_ENGINE_MY_TASK_HPP
#define SEARCH_ENGINE_MY_TASK_HPP

#include "tcpConnection.hpp"
#include <nlohmann/json_fwd.hpp>
#include <cstddef>
#include "dataCenter.hpp"
#include "runtimeConfig.hpp"
#include <iostream>
#include <vector>
#include <map>
#include <set>

class SplitTool;
class Cache;
class IndexManager;

class MyTask
{
public:
    using tcpConnectionPtr = std::shared_ptr<TcpConnection>;

public:
    // 请求消息、连接、分词器和后台索引管理器
    MyTask(const std::string &msg, const tcpConnectionPtr &con,
           std::shared_ptr<SplitTool> splitTool, std::shared_ptr<IndexManager> indexManager);
    ~MyTask();

    // 传入子线程cache
    void process(Cache &Cache);

private:
    void processQuery(Cache &cache);
    void respond(const nlohmann::json &value);

    // 最小编辑距离计算
    int editDistance(const std::string &lhs, const std::string &rhs);

    // 求字符串的长度
    std::size_t length(const std::string &str);

    // 求每个词占用的字节大小
    std::size_t nBytesCode(const char ch);

private:
    // 请求的信息
    std::string _msg;

    // tcp连接
    tcpConnectionPtr _con;

    // 网页库文件路径
    std::string _webPageFile;

    // jieba分词
    std::shared_ptr<SplitTool> _splitTool;
    std::shared_ptr<IndexManager> _indexManager;
    std::shared_ptr<const SearchData> _data;

private:
    struct WordCandidate
    {
        // 单词
        std::string _word;

        // 最小编辑距离
        int _distance;

        // 词频
        int _wordCnt;

        // 在单词表中的顺序
        int _index;

        bool operator<(const WordCandidate &rhs) const
        {
            // 先比较最小编辑距离
            if(_distance != rhs._distance)
            {
                return _distance < rhs._distance;
            }

            if(_wordCnt != rhs._wordCnt)
            {
                return _wordCnt > rhs._wordCnt;
            }

            return _index < rhs._index;
        }

        bool operator>(const WordCandidate &rhs) const
        {
            return rhs < *this;
        }

        // 打印信息确认
        void print()
        {
            std::cout << "-----------------------" << std::endl;
            std::cout << "word: " << _word << std::endl;
            std::cout << "distance: " << _distance << std::endl;
            std::cout << "wordCnt: " << _wordCnt << std::endl;
            std::cout << "index: " << _index << std::endl;
            std::cout << "-----------------------" << std::endl;
        }
    };
};

#endif
