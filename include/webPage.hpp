// WebPage.hpp
#pragma once

#include <simhash/Simhasher.hpp>
#include <vector>
#include <map>
#include <string>
#include <fstream>

using std::string;
using std::vector;
using std::map;
using std::pair;
using std::ifstream;
using namespace simhash;

class WebPage {
public:
    // 存放网页库的文件夹
    WebPage(const string &dirPath);
    ~WebPage();
    // 创建网页库文件
    void buildPage(const string &outFilePath);

private:
    // 根据文件夹获取所有文件的路径
    void saveFiles(const string &dirPath);
    // 读取一个文件中的网页内容，并进行清洗
    void readArticle(ifstream &ifs);
    // 获取网页的指纹值
    uint64_t calcFingerprint(const string &s1);
    // 比较两个指纹值是否相似
    bool compareFingerprint(uint64_t &u1, uint64_t &u2);
    // 将网页写入网页库文件，并记录每个网页在文件中的偏移位置和长度
    void storePage(const string &filePath);
    // 将每个网页在文件中的偏移位置和长度写入偏移文件
    void storeOffsetPage(const string &filePath);
    // 清洗字符串两端的空格
    void trim(string &s);
    // 清洗字符串中的标签、CDATA、空格等
    void cleanContent(string &s);
    // 清洗字符串中的CDATA、空格等
    void removeCDATA(string &s);

private:
    // 清洗后的整篇网页
    string _doc;
    // 网页id
    int _docId;
    // 网页标题
    string _docTitle;
    // 网页连接
    string _docUrl;
    // 网页内容
    string _docContent;
    // 网页库文件路径集合
    vector<string> _files;
    // 网页指纹第三方库, 用于提取网页的指纹
    Simhasher _simhasher;
    // 每个网页的指纹值
    vector<uint64_t> _fp;
    // 每个网页在网页库文件中的偏移位置和长度
    map<int, pair<size_t, size_t>> _newOffset;
};
