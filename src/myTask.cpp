// MyTask.cpp
#include "../include/myTask.hpp"
#include <nlohmann/json.hpp>
#include "../include/splitToolCppJieba.hpp"
#include "../include/cache.hpp"
#include <exception>
#include <fstream>
#include <queue>
#include <sstream>

using std::ifstream;
using std::istringstream;
using std::priority_queue;

// 请求的信息, tcp连接, 词典文件路径, 词典索引文件, 网页库文件, 网页偏移库文件, 倒排索引库文件
MyTask::MyTask(const string &msg, 
       const tcpConnectionPtr &con, 
       const string &dictFile, 
       const string &dictIndexFile, 
       const string &webPageFile,
       const string &offsetFile,
       const string &invertIndexFile)
: _msg(msg)
, _con(con)
, _webPageFile(webPageFile)
{
    if(DataCenter::getInstance()->_dict.empty())
    {
        // 加载磁盘信息 ditFile
        ifstream ifDict(dictFile);
        // 逐行读取即可
        string line;
        DataCenter::getInstance()->_dict.push_back({"0", 0}); // 没有第0行
        while(getline(ifDict, line))
        {
            string key;
            int value;
            istringstream iss(line);
            iss >> key >> value;
            DataCenter::getInstance()->_dict.push_back({key, value});
        }
        cout << "初始化词典完毕, 总共" << DataCenter::getInstance()->_dict.size() << "行" << endl;

        // 加载磁盘信息 dictIndexFile
        ifstream ifDictIndex(dictIndexFile);
        // 逐行读取即可
        while(getline(ifDictIndex, line))
        {
            string key;
            int value;
            istringstream iss(line);
            iss >> key;
            while(iss >> value)
            {
                DataCenter::getInstance()->_index[key].insert(value);
            }
        }
        cout << "初始化索引完毕, 总共" << DataCenter::getInstance()->_index.size() << "行" << endl;

        // 加载网页偏移库
        ifstream ifwebPageOffset(offsetFile);
        DataCenter::getInstance()->_webPageOffset.push_back({-1, -1}); // 没有id为0的
        // 逐行读取即可
        while(getline(ifwebPageOffset, line))
        {
            size_t docId, pos, len;
            istringstream iss(line);
            iss >> docId >> pos >> len;
            DataCenter::getInstance()->_webPageOffset.push_back({pos, len});
        }
        cout << "初始化网页偏移库完毕, 总共" << DataCenter::getInstance()->_webPageOffset.size() << "行" << endl;

        // 加载倒排索引库
        ifstream ifInvertIndex(invertIndexFile);
        // 逐行读取即可
        while(getline(ifInvertIndex, line))
        {
            // 单词
            string word;
            // 网页id
            int docId;
            // 单词权重
            double w;
            istringstream iss(line);
            iss >> word >> docId >> w;
            DataCenter::getInstance()->_invertedIndex[word].insert({docId, w});
        }
        cout << "初始化倒排索引库完毕, 总共" << DataCenter::getInstance()->_invertedIndex.size() << "词" << endl;
    }
    cout << "构造结束" << endl;
}

MyTask::~MyTask()
{

}

void MyTask::process(Cache &Cache)
{
    string msg;

    // 统一缓存: 先用 _msg 作为key查询缓存, type=1/2 都生效
    Cache.lock();
    msg = Cache.get(_msg);
    Cache.unlock();
    if(msg != "")
    {
        cout << "Cache hit, key=_msg" << endl;
        _con->sendInLoop(msg);
        return;
    }

    // 小火车协议:
    // receive()已剥离4字节长度头，_msg格式: [type(1B)][content]
    // 第1个字节代表类型, 1=关键字推荐, 2=网页查询
    // 后面是JSON信息
    cout << "now received msg size: " << _msg.size() << endl;
    char type = _msg[0];
    string content = _msg.substr(1);
    if(type == 1) // 处理关键字推荐模块
    {
        cout << "received content: " << content << endl;
        nlohmann::json json_object;
        try 
        {
            json_object = nlohmann::json::parse(content);
        }
        catch (const std::exception &e)
        {
            cout << "server JSON parse error (type=1): " << e.what() << endl;
            string err = "JSON parsing failed";
            int returnLen = err.size();
            string msg;
            msg.append((char *)&returnLen, 4);
            msg += err;
            _con->sendInLoop(msg);
            return;
        }
        string key = json_object["query"];
        for (auto &ch : key) { ch = tolower(ch); }
        cout << "received keyword: " << key << endl;
        // 对其进行分词, 然后获取并集
        set<int> can_index; 
        cout << "分词结果如下:" << endl;
        for(size_t idx = 0; idx != key.size(); ++idx)
        {
            size_t nBytes = nBytesCode(key[idx]);
            string word = key.substr(idx, nBytes);
            cout << word << endl;
            can_index.insert(DataCenter::getInstance()->_index[word].begin(), DataCenter::getInstance()->_index[word].end());  
            idx += (nBytes - 1);
        }
        cout << "并集大小是： " << can_index.size() << endl;

        // 对里面的数据进行关键字匹配
        // 先判断最小编辑距离, 再比较词频, 最后按照字母表顺序
        priority_queue<WordCandidate, vector<WordCandidate>, std::greater<WordCandidate>> pq;
        for(int elem : can_index)
        {
            WordCandidate w;
            w._word = DataCenter::getInstance()->_dict[elem].first;
            w._wordCnt = DataCenter::getInstance()->_dict[elem].second;
            w._index = elem;
            w._distance = editDistance(key, w._word);
            pq.push(w);
        }

        // 获取前5个单词给客户端
        int returnCnt = 5;
        // 封装返回的字符串
        string returnString;
        // 封装返回的json
        nlohmann::json return_json_array;
        while(!pq.empty() && returnCnt > 0)
        {
            --returnCnt;
            WordCandidate w = pq.top(); pq.pop();
            w.print();
            return_json_array.push_back(w._word);
        }
        returnString = return_json_array.dump();
        // 封装响应火车协议, 前4个代表发送的字节
        int returnLen = returnString.size();
        msg.append((char *)&returnLen, 4);
        msg += returnString;
        cout << "准备发送的信息是: " << msg << endl;
    }
    else // 网页搜索模块
    {
        cout << "收到的内容是: " << content << endl;
        // 解析json字符串, 这里规定类型是object, key是"query"
        nlohmann::json json_object;
        try
        {
            json_object = nlohmann::json::parse(content);
        }
        catch (const std::exception &e)
        {
            cout << "server JSON parse error (type=2): " << e.what() << endl;
            string err = "JSON parsing failed";
            int returnLen = err.size();
            string msg;
            msg.append((char *)&returnLen, 4);
            msg += err;
            _con->sendInLoop(msg);
            return;
        }
        // 获取到关键词
        string key = json_object["query"];
        for (auto &ch : key) { ch = tolower(ch); }
        cout << "收到的网页查询词是:" << key << endl;

        // 对其进行分词
        vector<string> words = _splitTool->cut(key);
        for (auto &word : words) { for (auto &ch : word) { ch = tolower(ch); } }

        // 获取所有文章id下的词的权重集合  
        map<int, vector<double>> res;
        int cnt = 0; // 遍历分词
        for(const auto &word : words)
        {
            set<pair<int, double>> s = DataCenter::getInstance()->_invertedIndex[word];
            if(s.empty())
            {
                cout << "word(" << word << ")这个词一个都没用,不用管\n";
                continue;
            }
            cout << "第" << ++cnt << "个分词是: " << word << endl;
            for(const auto &t : s)
            {
                res[t.first].push_back(t.second);
            }
        }
        // 取并集, 要求每个文章下都必须有这么多个词
        for (auto it = res.begin(); it != res.end(); ) 
        {  
            if (it->second.size() != cnt) 
            {
                it = res.erase(it);
            } 
            else 
            {
                ++it;
            }
        }
        cout << "有" << res.size() << "个候选的网页\n";

        // 需要获取文章id, 每个词的权重 总共x个文章, 每个文章y个词的权重
        // 1 将输入的内容内容当成网页, 先获取对应词的权重
        map<string, int>wordCnt; // 单词, 出现次数
        for(const auto & word : words)
        {
            ++wordCnt[word];
        }
        // 2 将每个词的权重全部放进去
        for(const auto &word : words)
        {
            // 某个词在某篇文章中出现的次数
            int TF = wordCnt[word];
            // 某个词在所有文章中的出现次数
            int DF = 1;
            // 文档总数
            int N = 1;
            // 逆文档频率
            double IDF = log( (double)N / (DF + 1) ) / log(2.0);
            // 单词权重
            double w = TF * IDF;
            // 基准向量是文章0
            res[0].push_back(w);
        }

        // 现在进行求余弦相似度, 相似度越大, 说明和文章越接近,  依次获取输入向量和所有文章向量的余弦值
        priority_queue<pair<double, int>, vector<pair<double, int>>, std::less<pair<double, int>>> pq;
        for(const auto &elem : res)
        {
            if(elem.first == 0)
            {
                cout << "不需要和自己比对向量余弦值\n";
                continue;
            }
            // 计算余弦值
            double dot = 0, lena = 0, lenb = 0;
            for(int i = 0; i < elem.second.size(); ++i)
            {
                dot += res[0][i] * elem.second[i];
                lena += res[0][i] * res[0][i];
                lenb += elem.second[i] * elem.second[i];
            }
            lena = sqrt(lena);
            lenb = sqrt(lenb);
            pq.push({dot / (lena * lenb), elem.first});
        }

        // 获取前10个网页给客户端
        int returnCnt = 10;
        // 封装返回的字符串
        string returnString;
        // 封装返回的json
        nlohmann::json return_json_array;
        while(!pq.empty() && returnCnt > 0)
        {
            --returnCnt;
            pair<double, int> docInfo= pq.top(); pq.pop();

            // 从网页偏移库中获取网页所在位置
            pair<size_t, size_t> now = DataCenter::getInstance()->_webPageOffset[docInfo.second];
            // 读取网页的内容
            ifstream ifswebPage(_webPageFile);

            // 跳转到指定位置
            ifswebPage.seekg(now.first, std::ios::beg);
            // 读取网页的内容 
            string doc;
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
        cout << "准备发送的信息是: " << msg << endl;

    }

    // 统一缓存: 将结果存入缓存, type=1/2 都生效
    Cache.lock();
    Cache.set(_msg, msg);
    Cache.setPending(_msg, msg);
    Cache.unlock();
    cout << "Cache set, key=_msg, cache_size=" << Cache.cache_size() << endl;

    _con->sendInLoop(msg);
}

// 最小编辑距离计算
int MyTask::editDistance(const std::string & lhs, const std::string &rhs)
{
    //计算最小编辑距离-包括处理中英文
    size_t lhs_len = length(lhs);
    size_t rhs_len = length(rhs);
    int editDist[lhs_len + 1][rhs_len + 1];
    for(size_t idx = 0; idx <= lhs_len; ++idx)
    {
        editDist[idx][0] = idx;
    }

    for(size_t idx = 0; idx <= rhs_len; ++idx)
    {
        editDist[0][idx] = idx;
    }

    std::string sublhs, subrhs;
    for(std::size_t dist_i = 1, lhs_idx = 0; dist_i <= lhs_len; ++dist_i, ++lhs_idx)
    {
        size_t nBytes = nBytesCode(lhs[lhs_idx]);
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
                editDist[dist_i][dist_j] = std::min({
                    editDist[dist_i - 1][dist_j] + 1,
                    editDist[dist_i][dist_j - 1] + 1,
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
size_t MyTask::nBytesCode(const char ch)
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
                break;
        }
        return nBytes;
    }
    return 1;
}

// 静态方法初始化
SplitTool* MyTask::_splitTool = new SplitToolCppJieba();
