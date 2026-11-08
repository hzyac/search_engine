// InvertedIndex.hpp
#pragma once

#include <string>
#include <map>
#include <set>

class SplitTool;

using std::string;
using std::map;
using std::set;
using std::pair;

class InvertedIndex {
public:
    // 加载网页文件路径, 偏移文件路径, 索引文件路径, 英文停用词文件路径, 中文停用词文件路径, 分词工具指针
    InvertedIndex(const string &pagePath, const string &offsetPath, const string &indexPath, const string &stopWordEnPath, const string &stopWordZhPath, SplitTool *SplitTool);
    ~InvertedIndex();
    // 构建倒排索引
    void buildInvertedIndex();
    // 写入倒排索引到文件中
    void storeInvertedIndex();

private:
    // 获取网页标题
    string getTitle(const string &s);
    // 获取网页内容
    string getContent(const string &s);

private:
    // 网页文件路径
    string _pagePath;
    // 偏移文件路径
    string _offsetPath;
    // 索引文件路径
    string _indexPath;
    // 英文停用词文件路径
    string _stopWordEnPath;
    // 中文停用词文件路径
    string _stopWordZhPath;
    // 分词工具指针
    SplitTool *_splitTool;
    // 倒排索引数据结构
    map<string, set<pair<int, double>>> _invertedIndex;
};
