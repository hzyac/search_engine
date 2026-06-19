// InvertedIndex.cpp
#include "../include/invertedIndex.hpp"
#include "../include/splitTool.hpp"
#include <cmath>
#include <iostream>
#include <fstream>
#include <sstream>

using std::ifstream;
using std::ofstream;
using std::ostringstream;
using std::cout;
using std::endl;

// 加载网页文件路径, 偏移文件路径, 索引文件路径, 英文停用词文件路径, 中文停用词文件路径, 分词工具指针
InvertedIndex::InvertedIndex(const string &pagePath, const string &offsetPath, const string &indexPath, const string &stopWordEnPath, const string &stopWordZhPath, SplitTool *SplitTool)
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
    ifstream ifs(_offsetPath);
    ifstream ifswebPage(_pagePath);
    ifstream ifsStopWordEn(_stopWordEnPath);
    ifstream ifsStopWordZh(_stopWordZhPath);

    map<string, map<int, int>> words;

    int docId;
    size_t pos, len;
    while(ifs >> docId >> pos >> len)
    {
        ifswebPage.seekg(pos);
        string doc(len, '\0');
        ifswebPage.read(&doc[0], len);

        string title = getTitle(doc);
        string content = getContent(doc);

        vector<string> titlewords = _splitTool->cut(title);
        vector<string> contentwords = _splitTool->cut(content);
        for (auto &word : titlewords) { for (auto &ch : word) { ch = tolower(ch); } }
        for (auto &word : contentwords) { for (auto &ch : word) { ch = tolower(ch); } }

        for(const auto &elem : titlewords)
        {
            ++words[elem][docId];
        }

        for(const auto &elem : contentwords)
        {
            ++words[elem][docId];
        }
    }
    cout << "read all articles done" << endl;
    
    string stopWordEn;
    int cnt = 0;
    while(getline(ifsStopWordEn, stopWordEn))
    {
        if (!stopWordEn.empty() && stopWordEn.back() == '\r') 
        {
            stopWordEn.pop_back();
        }
        if(words.find(stopWordEn) != words.end())
        {
            ++cnt;
            words.erase(stopWordEn);
        }
    }
    cout << "removed " << cnt << " English stop words" << endl;
    
    string stopWordZh;
    cnt = 0;
    while(getline(ifsStopWordZh, stopWordZh))
    {
        if (!stopWordZh.empty() && stopWordZh.back() == '\r') 
        {
            stopWordZh.pop_back();
        }
        if(words.find(stopWordZh) != words.end())
        {
            ++cnt;
            words.erase(stopWordZh);
        }
    }
    cout << "removed " << cnt << " Chinese stop words" << endl;

    vector<double> ws(docId + 1, 0);
    for(const auto &elem1 : words) 
    {
        for(const auto &elem2 : elem1.second)
        {
            int TF = elem2.second;
            int DF = elem1.second.size();
            int N = docId;
            double IDF = log( (double)N / (DF + 1) ) / log(2.0);
            double w = TF * IDF;
            _invertedIndex[elem1.first].insert({elem2.first, w});
            ws[elem2.first] += w * w;
        }
    }
    cout << "weight calculation done" << endl;

    for(auto &elem1 : _invertedIndex)
    {
        set<pair<int, double>> newans;
        for(auto &elem2 : elem1.second)
        {
            newans.insert({elem2.first, elem2.second / sqrt(ws[elem2.first])});
        }
        _invertedIndex[elem1.first] = newans;
    }
    cout << "normalization done" << endl;

    ifs.close();
    ifsStopWordEn.close();
    ifsStopWordZh.close();
    ifswebPage.close();
}

// 写入倒排索引到文件中
void InvertedIndex::storeInvertedIndex()
{
    ofstream ofs(_indexPath);
    ostringstream oss;
    for(const auto &elem1 : _invertedIndex)
    {
        for(const auto &elem2 : elem1.second)
        {
            oss << elem1.first << ' ' << elem2.first << ' ' << elem2.second << '\n';
        }
    }
    ofs << oss.str();
    ofs.close();
    cout << "inverted index file written" << endl;
}

// 获取网页标题
string InvertedIndex::getTitle(const string &s)
{
    string title;
    size_t t1 = s.find("<title>");
    size_t t2 = s.find("</title>");
    if (t1 != string::npos && t2 != string::npos)
    {
        title = s.substr(t1 + 7, t2 - (t1 + 7));
    }
    return title;
}

// 获取网页内容
string InvertedIndex::getContent(const string &s)
{
    string content;
    size_t c1 = s.find("<content>");
    size_t c2 = s.find("</content>");
    if (c1 != string::npos && c2 != string::npos)
    {
        content = s.substr(c1 + 9, c2 - (c1 + 9));
    }
    return content;
}