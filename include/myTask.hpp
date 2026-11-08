// MyTask.hpp
#pragma once
 
#include "tcpConnection.hpp"
#include "dataCenter.hpp"
#include <iostream>
#include <vector>
#include <map>
#include <set>

using std::vector;
using std::map;
using std::set;
using std::pair;
using std::cout;
using std::endl;

class SplitTool;
class Cache;

class MyTask {
public:
    using tcpConnectionPtr = shared_ptr<TcpConnection>;
public:
    // 请求的信息, tcp连接, 词典文件路径, 词典索引文件, 网页库文件, 网页偏移库文件, 倒排索引库文件
    MyTask(const string &msg, 
           const tcpConnectionPtr &con, 
           const string &dictFile, 
           const string &dictIndexFile, 
           const string &webPageFile,
           const string &offsetFile,
           const string &invertIndexFile);
    ~MyTask();
    // 传入子线程cache
    void process(Cache &Cache);

private:
    // 最小编辑距离计算
    int editDistance(const std::string & lhs, const std::string &rhs);
    // 求字符串的长度
    std::size_t length(const std::string &str);
    // 求每个词占用的字节大小
    size_t nBytesCode(const char ch);

private:
    // 请求的信息
    string _msg;
    // tcp连接
    tcpConnectionPtr _con; 
    // 网页库文件路径
    string _webPageFile;
    // jieba分词
    static SplitTool *_splitTool;

private:
    struct WordCandidate{
        // 单词
        string _word;
        // 最小编辑距离
        int _distance; 
        // 词频
        int _wordCnt;
        // 在单词表中的顺序
        int _index;
        bool operator<(const WordCandidate &rhs) const{
            // 先比较最小编辑距离
            if(_distance != rhs._distance) return _distance < rhs._distance;
            if(_wordCnt != rhs._wordCnt) return _wordCnt > rhs._wordCnt;
            return _index < rhs._index;
        }
        bool operator>(const WordCandidate &rhs) const {
            return rhs < *this;
        }
        // 打印信息确认
        void print()
        {
            cout << "-----------------------" << endl;
            cout << "word: " << _word << endl;
            cout << "distance: " << _distance << endl;
            cout << "wordCnt: " << _wordCnt << endl;
            cout << "index: " << _index << endl;
            cout << "-----------------------" << endl;
        }
    };
};
