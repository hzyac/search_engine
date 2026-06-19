// WebPage.cpp
#include "../include/webPage.hpp"
#include <cstdint>
#include <cstring>
#include <dirent.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <regex>

using std::ifstream;
using std::ofstream;
using std::ostringstream;
using std::cout;
using std::cerr;
using std::endl;
using std::regex;

// 存放网页库的文件夹
WebPage::WebPage(const string &dirPath)
: _docId(0)
, _simhasher("../include/simhash_dict/jieba.dict.utf8",   
             "../include/simhash_dict/hmm_model.utf8",    
             "../include/simhash_dict/idf.utf8",          
             "../include/simhash_dict/stop_words.utf8")
{
    saveFiles(dirPath);
}

WebPage::~WebPage()
{

}

// 根据文件夹获取所有文件的路径
void WebPage::saveFiles(const string &dirPath)
{
    DIR *dir = opendir(dirPath.c_str());
    if (!dir)
    {
        cerr << "open dir failed: " << dirPath << endl;
        return;
    }

    struct dirent *ptr;
    while ((ptr = readdir(dir)) != nullptr)
    {
        if (strcmp(ptr->d_name, ".") == 0 || strcmp(ptr->d_name, "..") == 0)
        {
            continue;
        }
        string filePath = dirPath + "/" + ptr->d_name;
        _files.push_back(filePath);
    }
    closedir(dir);
}

// 构建网页库文件
void WebPage::buildPage(const string &outFilePath)
{
    for(string &filePath : _files)
    {
        ifstream ifs(filePath);
        while(ifs)
        {
            _doc.clear(), _docTitle.clear(), _docUrl.clear(), _docContent.clear();
            readArticle(ifs);
            if(_doc.empty() || _docTitle.empty() || _docUrl.empty() || _docContent.empty())
            {
                --_docId;
                continue;
            }
            bool flag = false;
            uint64_t u = calcFingerprint(_doc);
            for(int i = 0; i <_fp.size(); ++i)
            {
                if(compareFingerprint(u, _fp[i]) == true)
                {
                    flag = true;
                    break;
                }
            }
            if(flag)
            {
                --_docId;
                continue;
            }
            _fp.push_back(u);
            storePage(outFilePath);
        }
        ifs.close();
        cout << "corpus written: " << filePath << "\t->\t" << outFilePath << endl;
    }
    storeOffsetPage("../data/newoffset.dat");
}

// 读取一个文件中的网页内容，并进行清洗
void WebPage::readArticle(ifstream &ifs)
{
    string line;
    while (getline(ifs, line))
    {
        trim(line);

        size_t pos;
        if ((pos = line.find("<title>")) != string::npos)
        {
            size_t endp = line.find("</title>");
            _docTitle = line.substr(pos + 7, endp - pos - 7);
            removeCDATA(_docTitle);
            trim(_docTitle);
        }
        else if ((pos = line.find("<link>")) != string::npos)
        {
            size_t endp = line.find("</link>");
            _docUrl = line.substr(pos + 6, endp - pos - 6);
            trim(_docUrl);
        }
        else if ((pos = line.find("<description>")) != string::npos)
        {
            size_t endp = line.find("</description>");
            _docContent = line.substr(pos + 13, endp - pos - 13);
            cleanContent(_docContent);
        }
        else if (line == "</item>")
        {
            ++_docId;
            trim(_docTitle), trim(_docContent), trim(_docUrl);
            ostringstream oss;
            oss
            << "<doc>\n"
            << "\t<id>" << _docId << "</id>\n" 
            << "\t<url>" << _docUrl << "</url>\n" 
            << "\t<title>" << _docTitle << "</title>\n"
            << "\t<content>" << _docContent << "</content>\n"
            << "</doc>\n\n"; 
            _doc = oss.str();
            return;
        }
    }
}

// 获取网页的指纹值
uint64_t WebPage::calcFingerprint(const string &s)
{
    uint64_t u;
    _simhasher.make(s, 15, u);
    return u;
}

// 比较两个指纹值是否相似
bool WebPage::compareFingerprint(uint64_t &u1, uint64_t &u2)
{
    return Simhasher::isEqual(u1, u2);
}

// 将网页写入网页库文件，并记录每个网页在文件中的偏移位置和长度
void WebPage::storePage(const string &filePath)
{
    ofstream ofs(filePath, std::ios::app);
    if (!ofs)
    {
        cerr << "open page file failed!" << endl;
        return;
    }
    _newOffset[_docId] = {(size_t)ofs.tellp(), (size_t)_doc.size()};

    ofs << _doc;
    ofs.flush();
    ofs.close();
}

// 将每个网页在文件中的偏移位置和长度写入偏移文件
void WebPage::storeOffsetPage(const string &filePath)
{
    ofstream ofs(filePath);
    for(const auto &elem : _newOffset)
    {
        ofs << elem.first << ' ' << elem.second.first << ' ' << elem.second.second << endl;
    }
    ofs.close();
    cout << "offset file written: " << filePath << endl;
}

// 清洗字符串两端的空格
void WebPage::trim(string &s)
{
    regex fullWidthSpace(u8"\u3000");
    s = regex_replace(s, fullWidthSpace, " ");

    auto start = s.begin();
    while (start != s.end() && isspace(static_cast<unsigned char>(*start)))
        start++;

    auto end = s.end();
    do
        end--;
    while (distance(start, end) > 0 && isspace(static_cast<unsigned char>(*end)));

    s = string(start, end + 1);
}

// 清洗字符串中的标签、CDATA、空格等
void WebPage::cleanContent(string &s)
{
    regex tagPat("<[^>]+>");
    s = regex_replace(s, tagPat, "");
    removeCDATA(s);
    regex spacePat("\\s+");
    s = regex_replace(s, spacePat, " ");
    trim(s);
}

// 清洗字符串中的CDATA、空格等
void WebPage::removeCDATA(string &s)
{
    size_t pos = 0;
    while((pos = s.find("<![CDATA[", pos)) != string::npos)
    {
        s.erase(pos, 9);
    }
    pos = 0;
    while((pos = s.find("]]>", pos)) != string::npos)
    {
        s.erase(pos, 3);
    }
    pos = 0;
    while((pos = s.find("&nbsp;", pos)) != string::npos)
    {
        s.erase(pos, 6);
    }
}