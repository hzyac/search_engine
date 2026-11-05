// InvertedIndex.hpp
#ifndef SEARCH_ENGINE_INVERTED_INDEX_HPP
#define SEARCH_ENGINE_INVERTED_INDEX_HPP

#include "document.hpp"
#include <string>
#include <map>
#include <set>

class SplitTool;

class InvertedIndex
{
public:
    // 加载网页文件路径, 偏移文件路径, 索引文件路径, 英文停用词文件路径, 中文停用词文件路径, 分词工具指针
    InvertedIndex(const std::string &pagePath, const std::string &offsetPath,
                  const std::string &indexPath, const std::string &stopWordEnPath,
                  const std::string &stopWordZhPath, SplitTool *SplitTool);
    ~InvertedIndex();

    // 构建倒排索引
    void buildInvertedIndex();
    void buildInvertedIndex(const std::vector<Document> &documents);

    // 写入倒排索引到文件中
    void storeInvertedIndex();

private:
    // 获取网页标题
    std::string getTitle(const std::string &s);

    // 获取网页内容
    std::string getContent(const std::string &s);

private:
    // 网页文件路径
    std::string _pagePath;

    // 偏移文件路径
    std::string _offsetPath;

    // 索引文件路径
    std::string _indexPath;

    // 英文停用词文件路径
    std::string _stopWordEnPath;

    // 中文停用词文件路径
    std::string _stopWordZhPath;

    // 分词工具指针
    SplitTool *_splitTool;

    // 倒排索引数据结构
    std::map<std::string, std::set<std::pair<int, double>>> _invertedIndex;
};

#endif
