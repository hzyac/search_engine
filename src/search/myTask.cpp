// MyTask.cpp
#include "myTask.hpp"
#include "indexManager.hpp"
#include <nlohmann/json.hpp>
#include "splitTool.hpp"
#include <cmath>
#include <cctype>
#include <stdexcept>
#include "cache.hpp"
#include <exception>
#include <fstream>
#include <queue>
#include <sstream>

// 请求消息、连接、分词器和后台索引管理器
MyTask::MyTask(const std::string &msg, const tcpConnectionPtr &con,
               std::shared_ptr<SplitTool> splitTool, std::shared_ptr<IndexManager> indexManager)
: _msg(msg)
, _con(con)
, _splitTool(std::move(splitTool))
, _indexManager(std::move(indexManager))
{
}

MyTask::~MyTask()
{
}

void MyTask::respond(const nlohmann::json &value)
{
    const auto body = value.dump();
    const auto length = static_cast<std::int32_t>(body.size());
    std::string message(reinterpret_cast<const char *>(&length), sizeof(length));
    message += body;
    _con->sendInLoop(message);
}

void MyTask::process(Cache &cache)
{
    try
    {
        if(_msg.empty())
        {
            throw std::runtime_error("Empty request");
        }

        const auto type = static_cast<unsigned char>(_msg[0]);
        if(type == 3 || type == 4)
        {
            const auto request = nlohmann::json::parse(_msg.substr(1));
            respond(type == 3 ? _indexManager->submit(request) : _indexManager->status(request));
            return;
        }

        _data = DataCenter::getInstance()->snapshot();
        _webPageFile = _data->_config.dataFile("newripepage.dat");
        processQuery(cache);
    }
    catch(const std::exception &error)
    {
        respond({{"error", error.what()}});
    }
}

void MyTask::processQuery(Cache &Cache)
{
    std::string msg;
    if(_msg.empty() || (_msg[0] != 1 && _msg[0] != 2))
    {
        const std::string error = nlohmann::json{{"error", "Invalid request type"}}.dump();
        const int size = static_cast<int>(error.size());
        msg.append(reinterpret_cast<const char *>(&size), 4);
        msg += error;
        _con->sendInLoop(msg);
        return;
    }

    const auto cacheKey = _data->_generation + ":" + _msg;

    // 缓存绑定索引版本，旧版本结果不会用于新索引。
    Cache.lock();
    msg = Cache.get(cacheKey);
    Cache.unlock();
    if(msg != "")
    {
        std::cout << "Cache hit, key=_msg" << std::endl;
        _con->sendInLoop(msg);
        return;
    }

    // 小火车协议:
    // receive()已剥离4字节长度头，_msg格式: [type(1B)][content]
    // 第1个字节代表类型, 1=关键字推荐, 2=网页查询
    // 后面是JSON信息
    std::cout << "now received msg size: " << _msg.size() << std::endl;
    char type = _msg[0];
    std::string content = _msg.substr(1);
    if(type == 1) // 处理关键字推荐模块
    {
        std::cout << "received content: " << content << std::endl;
        nlohmann::json json_object;
        try
        {
            json_object = nlohmann::json::parse(content);
            if(!json_object.is_object() || !json_object.contains("query") ||
               !json_object["query"].is_string())
            {
                throw std::runtime_error("query must be a string");
            }
        }
        catch(const std::exception &e)
        {
            std::cout << "server JSON parse error (type=1): " << e.what() << std::endl;
            std::string err =
                nlohmann::json{{"error", "query must be valid JSON with a string query"}}.dump();
            int returnLen = err.size();
            std::string msg;
            msg.append((char *)&returnLen, 4);
            msg += err;
            _con->sendInLoop(msg);
            return;
        }

        std::string key = json_object["query"];
        for(auto &ch : key)
        {
            ch = std::tolower(static_cast<unsigned char>(ch));
        }

        std::cout << "received keyword: " << key << std::endl;

        // 对其进行分词, 然后获取并集
        std::set<int> can_index;
        std::cout << "分词结果如下:" << std::endl;
        for(std::size_t idx = 0; idx != key.size(); ++idx)
        {
            std::size_t nBytes = nBytesCode(key[idx]);
            std::string word = key.substr(idx, nBytes);
            std::cout << word << std::endl;
            const auto &index = _data->_index;
            const auto it = index.find(word);
            if(it != index.end())
            {
                can_index.insert(it->second.begin(), it->second.end());
            }

            idx += (nBytes - 1);
        }

        std::cout << "并集大小是： " << can_index.size() << std::endl;

        // 对里面的数据进行关键字匹配
        // 先判断最小编辑距离, 再比较词频, 最后按照字母表顺序
        std::priority_queue<WordCandidate, std::vector<WordCandidate>, std::greater<WordCandidate>>
            pq;
        for(int elem : can_index)
        {
            WordCandidate w;
            w._word = _data->_dict[elem].first;
            w._wordCnt = _data->_dict[elem].second;
            w._index = elem;
            w._distance = editDistance(key, w._word);
            pq.push(w);
        }

        // 获取前5个单词给客户端
        int returnCnt = 5;

        // 封装返回的字符串
        std::string returnString;

        // 封装返回的json
        nlohmann::json return_json_array = nlohmann::json::array();
        while(!pq.empty() && returnCnt > 0)
        {
            --returnCnt;
            WordCandidate w = pq.top();
            pq.pop();
            w.print();
            return_json_array.push_back(w._word);
        }

        returnString = return_json_array.dump();

        // 封装响应火车协议, 前4个代表发送的字节
        int returnLen = returnString.size();
        msg.append((char *)&returnLen, 4);
        msg += returnString;
        std::cout << "准备发送的信息是: " << msg << std::endl;
    }
    else // 网页搜索模块
    {
        std::cout << "收到的内容是: " << content << std::endl;

        // 解析json字符串, 这里规定类型是object, key是"query"
        nlohmann::json json_object;
        try
        {
            json_object = nlohmann::json::parse(content);
            if(!json_object.is_object() || !json_object.contains("query") ||
               !json_object["query"].is_string())
            {
                throw std::runtime_error("query must be a string");
            }
        }
        catch(const std::exception &e)
        {
            std::cout << "server JSON parse error (type=2): " << e.what() << std::endl;
            std::string err =
                nlohmann::json{{"error", "query must be valid JSON with a string query"}}.dump();
            int returnLen = err.size();
            std::string msg;
            msg.append((char *)&returnLen, 4);
            msg += err;
            _con->sendInLoop(msg);
            return;
        }

        // 获取到关键词
        std::string key = json_object["query"];
        for(auto &ch : key)
        {
            ch = std::tolower(static_cast<unsigned char>(ch));
        }

        std::cout << "收到的网页查询词是:" << key << std::endl;

        // 对其进行分词
        std::vector<std::string> words = _splitTool->cut(key);
        for(auto &word : words)
        {
            for(auto &ch : word)
            {
                ch = std::tolower(static_cast<unsigned char>(ch));
            }
        }

        // Use the same smoothed IDF as offline indexing. Keep query and
        // document dimensions aligned, including repeated/unknown tokens.
        const auto *data = _data.get();
        std::map<std::string, int> frequencies;
        for(const auto &word : words)
        {
            if(data->_invertedIndex.find(word) != data->_invertedIndex.end())
            {
                ++frequencies[word];
            }
        }

        std::map<int, std::vector<double>> res;
        std::vector<double> queryWeights;
        const auto documentCount = data->_webPageOffset.size() - 1;
        for(const auto &entry : frequencies)
        {
            const auto &postings = data->_invertedIndex.at(entry.first);
            queryWeights.push_back(
                entry.second * (std::log2((documentCount + 1.0) / (postings.size() + 1.0)) + 1.0));
            for(const auto &posting : postings)
            {
                res[posting.first].push_back(posting.second);
            }
        }

        for(auto it = res.begin(); it != res.end();)
        {
            if(it->second.size() != queryWeights.size())
            {
                it = res.erase(it);
            }
            else
            {
                ++it;
            }
        }

        res[0] = std::move(queryWeights);

        // 现在进行求余弦相似度, 相似度越大, 说明和文章越接近,  依次获取输入向量和所有文章向量的余弦值
        std::priority_queue<std::pair<double, int>, std::vector<std::pair<double, int>>,
                            std::less<std::pair<double, int>>>
            pq;
        for(const auto &elem : res)
        {
            if(elem.first == 0)
            {
                std::cout << "不需要和自己比对向量余弦值\n";
                continue;
            }

            // 计算余弦值
            double dot = 0, lena = 0, lenb = 0;
            for(std::size_t i = 0; i < elem.second.size(); ++i)
            {
                dot += res[0][i] * elem.second[i];
                lena += res[0][i] * res[0][i];
                lenb += elem.second[i] * elem.second[i];
            }

            lena = std::sqrt(lena);
            lenb = std::sqrt(lenb);
            if(lena > 0 && lenb > 0)
            {
                pq.push({dot / (lena * lenb), elem.first});
            }
        }

        // 获取前10个网页给客户端
        int returnCnt = 10;

        // 封装返回的字符串
        std::string returnString;

        // 封装返回的json
        nlohmann::json return_json_array = nlohmann::json::array();
        while(!pq.empty() && returnCnt > 0)
        {
            --returnCnt;
            std::pair<double, int> docInfo = pq.top();
            pq.pop();

            // 从网页偏移库中获取网页所在位置
            std::pair<std::size_t, std::size_t> now = _data->_webPageOffset[docInfo.second];

            // 读取网页的内容
            std::ifstream ifswebPage(_webPageFile);

            // 跳转到指定位置
            ifswebPage.seekg(now.first, std::ios::beg);

            // 读取网页的内容
            std::string doc;
            doc.resize(now.second, '\0');
            ifswebPage.read(&doc[0], now.second);
            doc.erase(doc.find_last_not_of("\n\r") + 1);

            // 存入信息
            return_json_array.push_back(doc);
        }

        if(return_json_array.empty())
        {
            return_json_array.push_back("抱歉, 没有找到相关数据!");
        }

        returnString = return_json_array.dump();

        // 封装响应火车协议, 前4个代表发送的字节
        int returnLen = returnString.size();
        msg.append((char *)&returnLen, 4);
        msg += returnString;
        std::cout << "准备发送的信息是: " << msg << std::endl;
    }

    // 统一缓存: 将结果存入缓存, type=1/2 都生效
    Cache.lock();
    Cache.set(cacheKey, msg);
    Cache.setPending(cacheKey, msg);
    Cache.unlock();
    std::cout << "Cache set, key=_msg, cache_size=" << Cache.cache_size() << std::endl;

    _con->sendInLoop(msg);
}

// 最小编辑距离计算
int MyTask::editDistance(const std::string &lhs, const std::string &rhs)
{
    //计算最小编辑距离-包括处理中英文
    std::size_t lhs_len = length(lhs);
    std::size_t rhs_len = length(rhs);
    std::vector<std::vector<int>> editDist(lhs_len + 1, std::vector<int>(rhs_len + 1));
    for(std::size_t idx = 0; idx <= lhs_len; ++idx)
    {
        editDist[idx][0] = idx;
    }

    for(std::size_t idx = 0; idx <= rhs_len; ++idx)
    {
        editDist[0][idx] = idx;
    }

    std::string sublhs, subrhs;
    for(std::size_t dist_i = 1, lhs_idx = 0; dist_i <= lhs_len; ++dist_i, ++lhs_idx)
    {
        std::size_t nBytes = nBytesCode(lhs[lhs_idx]);
        sublhs = lhs.substr(lhs_idx, nBytes);
        lhs_idx += (nBytes - 1);

        for(std::size_t dist_j = 1, rhs_idx = 0; dist_j <= rhs_len; ++dist_j, ++rhs_idx)
        {
            nBytes = nBytesCode(rhs[rhs_idx]);
            subrhs = rhs.substr(rhs_idx, nBytes);
            rhs_idx += (nBytes - 1);
            if(sublhs == subrhs)
            {
                editDist[dist_i][dist_j] = editDist[dist_i - 1][dist_j - 1];
            }
            else
            {
                editDist[dist_i][dist_j] =
                    std::min({editDist[dist_i - 1][dist_j] + 1, editDist[dist_i][dist_j - 1] + 1,
                              editDist[dist_i - 1][dist_j - 1] + 1});
            }
        }
    }

    return editDist[lhs_len][rhs_len];
}

// 求字符串的长度
std::size_t MyTask::length(const std::string &str)
{
    std::size_t ilen = 0;
    for(std::size_t idx = 0; idx != str.size(); ++idx)
    {
        int nBytes = nBytesCode(str[idx]);
        idx += (nBytes - 1);
        ++ilen;
    }

    return ilen;
}

// 求每个词占用的字节大小
std::size_t MyTask::nBytesCode(const char ch)
{
    if(ch & (1 << 7))
    {
        int nBytes = 1;
        for(int idx = 0; idx != 6; ++idx)
        {
            if(ch & (1 << (6 - idx)))
            {
                ++nBytes;
            }
            else
            {
                break;
            }
        }

        return nBytes;
    }

    return 1;
}

// 静态方法初始化
