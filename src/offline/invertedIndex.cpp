// InvertedIndex.cpp
#include "invertedIndex.hpp"
#include <cctype>
#include "splitTool.hpp"
#include <cmath>
#include <stdexcept>
#include <iostream>
#include <fstream>
#include <sstream>

// 加载网页文件路径, 偏移文件路径, 索引文件路径, 英文停用词文件路径, 中文停用词文件路径, 分词工具指针
InvertedIndex::InvertedIndex(const std::string &pagePath, const std::string &offsetPath,
                             const std::string &indexPath, const std::string &stopWordEnPath,
                             const std::string &stopWordZhPath, SplitTool *SplitTool)
: _pagePath(pagePath)
, _offsetPath(offsetPath)
, _indexPath(indexPath)
, _stopWordEnPath(stopWordEnPath)
, _stopWordZhPath(stopWordZhPath)
, _splitTool(SplitTool)
{
}

InvertedIndex::~InvertedIndex()
{
}

// 构建倒排索引
void InvertedIndex::buildInvertedIndex()
{
    std::ifstream ifs(_offsetPath);
    std::ifstream ifswebPage(_pagePath);
    std::vector<Document> documents;
    int id;
    std::size_t position, length;
    while(ifs >> id >> position >> length)
    {
        ifswebPage.seekg(position);
        std::string page(length, '\0');
        ifswebPage.read(page.data(), length);
        documents.push_back({std::to_string(id), getTitle(page), {}, getContent(page)});
    }

    buildInvertedIndex(documents);
}

void InvertedIndex::buildInvertedIndex(const std::vector<Document> &documents)
{
    _invertedIndex.clear();
    std::ifstream ifsStopWordEn(_stopWordEnPath);
    std::ifstream ifsStopWordZh(_stopWordZhPath);
    std::map<std::string, std::map<int, int>> words;

    int docId = 0;
    for(const auto &document : documents)
    {
        ++docId;
        const auto &title = document.title;
        const auto &content = document.content;

        std::vector<std::string> titlewords = _splitTool->cut(title);
        std::vector<std::string> contentwords = _splitTool->cut(content);
        for(auto &word : titlewords)
        {
            for(auto &ch : word)
            {
                ch = std::tolower(static_cast<unsigned char>(ch));
            }
        }

        for(auto &word : contentwords)
        {
            for(auto &ch : word)
            {
                ch = std::tolower(static_cast<unsigned char>(ch));
            }
        }

        for(const auto &elem : titlewords)
        {
            if(!elem.empty() && elem.find_first_not_of(" \t\r\n") != std::string::npos)
            {
                ++words[elem][docId];
            }
        }

        for(const auto &elem : contentwords)
        {
            if(!elem.empty() && elem.find_first_not_of(" \t\r\n") != std::string::npos)
            {
                ++words[elem][docId];
            }
        }
    }

    std::cout << "read all articles done" << std::endl;

    std::string stopWordEn;
    int cnt = 0;
    while(std::getline(ifsStopWordEn, stopWordEn))
    {
        if(!stopWordEn.empty() && stopWordEn.back() == '\r')
        {
            stopWordEn.pop_back();
        }

        if(words.find(stopWordEn) != words.end())
        {
            ++cnt;
            words.erase(stopWordEn);
        }
    }

    std::cout << "removed " << cnt << " English stop words" << std::endl;

    std::string stopWordZh;
    cnt = 0;
    while(std::getline(ifsStopWordZh, stopWordZh))
    {
        if(!stopWordZh.empty() && stopWordZh.back() == '\r')
        {
            stopWordZh.pop_back();
        }

        if(words.find(stopWordZh) != words.end())
        {
            ++cnt;
            words.erase(stopWordZh);
        }
    }

    std::cout << "removed " << cnt << " Chinese stop words" << std::endl;

    if(docId == 0)
    {
        throw std::runtime_error("No documents in webpage offsets");
    }

    std::vector<double> ws(docId + 1, 0);
    for(const auto &elem1 : words)
    {
        for(const auto &elem2 : elem1.second)
        {
            int TF = elem2.second;
            int DF = elem1.second.size();
            int N = docId;
            double IDF = std::log2((N + 1.0) / (DF + 1.0)) + 1.0;
            double w = TF * IDF;
            _invertedIndex[elem1.first].insert({elem2.first, w});
            ws[elem2.first] += w * w;
        }
    }

    std::cout << "weight calculation done" << std::endl;

    for(auto &elem1 : _invertedIndex)
    {
        std::set<std::pair<int, double>> newans;
        for(auto &elem2 : elem1.second)
        {
            newans.insert({elem2.first,
                           ws[elem2.first] > 0 ? elem2.second / std::sqrt(ws[elem2.first]) : 0.0});
        }

        _invertedIndex[elem1.first] = newans;
    }

    std::cout << "normalization done" << std::endl;

    ifsStopWordEn.close();
    ifsStopWordZh.close();
}

// 写入倒排索引到文件中
void InvertedIndex::storeInvertedIndex()
{
    std::ofstream ofs(_indexPath);
    if(!ofs)
    {
        throw std::runtime_error("Cannot write inverted index: " + _indexPath);
    }

    std::ostringstream oss;
    for(const auto &elem1 : _invertedIndex)
    {
        for(const auto &elem2 : elem1.second)
        {
            oss << elem1.first << ' ' << elem2.first << ' ' << elem2.second << '\n';
        }
    }

    ofs << oss.str();
    ofs.close();
    std::cout << "inverted index file written" << std::endl;
}

// 获取网页标题
std::string InvertedIndex::getTitle(const std::string &s)
{
    std::string title;
    std::size_t t1 = s.find("<title>");
    std::size_t t2 = s.find("</title>");
    if(t1 != std::string::npos && t2 != std::string::npos)
    {
        title = s.substr(t1 + 7, t2 - (t1 + 7));
    }

    return title;
}

// 获取网页内容
std::string InvertedIndex::getContent(const std::string &s)
{
    std::string content;
    std::size_t c1 = s.find("<content>");
    std::size_t c2 = s.find("</content>");
    if(c1 != std::string::npos && c2 != std::string::npos)
    {
        content = s.substr(c1 + 9, c2 - (c1 + 9));
    }

    return content;
}
