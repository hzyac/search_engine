// WebPage.hpp
#ifndef SEARCH_ENGINE_WEB_PAGE_HPP
#define SEARCH_ENGINE_WEB_PAGE_HPP

#include <simhash/Simhasher.hpp>
#include <cstddef>
#include <cstdint>
#include "runtimeConfig.hpp"
#include <vector>
#include <map>
#include <string>
#include <fstream>

class WebPage
{
public:
    // 存放网页库的文件夹
    WebPage(const std::string &dirPath, const RuntimeConfig &config);
    ~WebPage();

    // 创建网页库文件
    void buildPage(const std::string &outFilePath, const std::string &offsetFilePath);

private:
    // 根据文件夹获取所有文件的路径
    void saveFiles(const std::string &dirPath);

    // 读取一个文件中的网页内容，并进行清洗
    void readArticle(std::ifstream &ifs);

    // 获取网页的指纹值
    std::uint64_t calcFingerprint(const std::string &s1);

    // 比较两个指纹值是否相似
    bool compareFingerprint(std::uint64_t &u1, std::uint64_t &u2);

    // 将网页写入网页库文件，并记录每个网页在文件中的偏移位置和长度
    void storePage(const std::string &filePath);

    // 将每个网页在文件中的偏移位置和长度写入偏移文件
    void storeOffsetPage(const std::string &filePath);

    // 清洗字符串两端的空格
    void trim(std::string &s);

    // 清洗字符串中的标签、CDATA、空格等
    void cleanContent(std::string &s);

    // 清洗字符串中的CDATA、空格等
    void removeCDATA(std::string &s);

private:
    // 清洗后的整篇网页
    std::string _doc;

    // 网页id
    int _docId;

    // 网页标题
    std::string _docTitle;

    // 网页连接
    std::string _docUrl;

    // 网页内容
    std::string _docContent;

    // 网页库文件路径集合
    std::vector<std::string> _files;

    // 网页指纹第三方库, 用于提取网页的指纹
    simhash::Simhasher _simhasher;

    // 每个网页的指纹值
    std::vector<std::uint64_t> _fp;

    // 每个网页在网页库文件中的偏移位置和长度
    std::map<int, std::pair<std::size_t, std::size_t>> _newOffset;
};

#endif
