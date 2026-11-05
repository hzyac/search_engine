// WebPage.cpp
#include "webPage.hpp"
#include <cstdint>
#include <cstring>
#include <dirent.h>
#include <algorithm>
#include <filesystem>
#include <stdexcept>
#include <iostream>
#include <fstream>
#include <sstream>
#include <regex>

// 存放网页库的文件夹
WebPage::WebPage(const std::string &dirPath, const RuntimeConfig &config)
: _docId(0)
, _simhasher((config.jiebaDir / "jieba.dict.utf8").string(),
             (config.jiebaDir / "hmm_model.utf8").string(), (config.jiebaDir / "idf.utf8").string(),
             (config.jiebaDir / "stop_words.utf8").string())
{
    saveFiles(dirPath);
}

WebPage::~WebPage()
{
}

// 根据文件夹获取所有文件的路径
void WebPage::saveFiles(const std::string &dirPath)
{
    DIR *dir = opendir(dirPath.c_str());
    if(!dir)
    {
        std::cerr << "open dir failed: " << dirPath << std::endl;
        return;
    }

    struct dirent *ptr;
    while((ptr = readdir(dir)) != nullptr)
    {
        if(strcmp(ptr->d_name, ".") == 0 || strcmp(ptr->d_name, "..") == 0)
        {
            continue;
        }

        std::string filePath = dirPath + "/" + ptr->d_name;
        if(std::filesystem::is_regular_file(filePath))
        {
            _files.push_back(filePath);
        }
    }

    closedir(dir);
    std::sort(_files.begin(), _files.end());
    if(_files.empty())
    {
        throw std::runtime_error("No input files in: " + dirPath);
    }
}

// 构建网页库文件
void WebPage::buildPage(const std::string &outFilePath, const std::string &offsetFilePath)
{
    std::ofstream reset(outFilePath, std::ios::trunc);
    if(!reset)
    {
        throw std::runtime_error("Cannot write: " + outFilePath);
    }

    reset.close();
    _docId = 0;
    _fp.clear();
    _newOffset.clear();
    for(std::string &filePath : _files)
    {
        std::ifstream ifs(filePath);
        while(ifs)
        {
            _doc.clear();
            _docTitle.clear();
            _docUrl.clear();
            _docContent.clear();

            readArticle(ifs);
            if(_doc.empty())
            {
                break;
            }

            if(_doc.empty() || _docTitle.empty() || _docUrl.empty() || _docContent.empty())
            {
                --_docId;
                continue;
            }

            bool flag = false;
            std::uint64_t u = calcFingerprint(_doc);
            for(std::size_t i = 0; i < _fp.size(); ++i)
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
        std::cout << "corpus written: " << filePath << "\t->\t" << outFilePath << std::endl;
    }

    storeOffsetPage(offsetFilePath);
}

// 读取一个文件中的网页内容，并进行清洗
void WebPage::readArticle(std::ifstream &ifs)
{
    std::string line;
    while(std::getline(ifs, line))
    {
        trim(line);

        std::size_t pos;
        if((pos = line.find("<title>")) != std::string::npos)
        {
            std::size_t endp = line.find("</title>");
            _docTitle = line.substr(pos + 7, endp - pos - 7);
            removeCDATA(_docTitle);
            trim(_docTitle);
        }
        else if((pos = line.find("<link>")) != std::string::npos)
        {
            std::size_t endp = line.find("</link>");
            _docUrl = line.substr(pos + 6, endp - pos - 6);
            trim(_docUrl);
        }
        else if((pos = line.find("<description>")) != std::string::npos)
        {
            std::size_t endp = line.find("</description>");
            _docContent = line.substr(pos + 13, endp - pos - 13);
            cleanContent(_docContent);
        }
        else if(line == "</item>")
        {
            ++_docId;
            trim(_docTitle);
            trim(_docContent);
            trim(_docUrl);

            std::ostringstream oss;
            oss << "<doc>\n"
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
std::uint64_t WebPage::calcFingerprint(const std::string &s)
{
    std::uint64_t u = 0;
    if(!_simhasher.make(s, 15, u))
    {
        throw std::runtime_error("Simhash fingerprint calculation failed");
    }

    return u;
}

// 比较两个指纹值是否相似
bool WebPage::compareFingerprint(std::uint64_t &u1, std::uint64_t &u2)
{
    return simhash::Simhasher::isEqual(u1, u2);
}

// 将网页写入网页库文件，并记录每个网页在文件中的偏移位置和长度
void WebPage::storePage(const std::string &filePath)
{
    std::ofstream ofs(filePath, std::ios::app);
    if(!ofs)
    {
        throw std::runtime_error("Cannot write webpage file: " + filePath);
    }

    _newOffset[_docId] = {(std::size_t)ofs.tellp(), (std::size_t)_doc.size()};

    ofs << _doc;
    ofs.flush();
    ofs.close();
}

// 将每个网页在文件中的偏移位置和长度写入偏移文件
void WebPage::storeOffsetPage(const std::string &filePath)
{
    std::ofstream ofs(filePath);
    for(const auto &elem : _newOffset)
    {
        ofs << elem.first << ' ' << elem.second.first << ' ' << elem.second.second << std::endl;
    }

    ofs.close();
    std::cout << "offset file written: " << filePath << std::endl;
}

// 清洗字符串两端的空格
void WebPage::trim(std::string &s)
{
    std::regex fullWidthSpace(u8"\u3000");
    s = std::regex_replace(s, fullWidthSpace, " ");

    auto first = s.find_first_not_of(" \t\r\n");
    if(first == std::string::npos)
    {
        s.clear();
        return;
    }

    s = s.substr(first, s.find_last_not_of(" \t\r\n") - first + 1);
}

// 清洗字符串中的标签、CDATA、空格等
void WebPage::cleanContent(std::string &s)
{
    std::regex tagPat("<[^>]+>");
    s = std::regex_replace(s, tagPat, "");
    removeCDATA(s);
    std::regex spacePat("\\s+");
    s = std::regex_replace(s, spacePat, " ");
    trim(s);
}

// 清洗字符串中的CDATA、空格等
void WebPage::removeCDATA(std::string &s)
{
    std::size_t pos = 0;
    while((pos = s.find("<![CDATA[", pos)) != std::string::npos)
    {
        s.erase(pos, 9);
    }

    pos = 0;
    while((pos = s.find("]]>", pos)) != std::string::npos)
    {
        s.erase(pos, 3);
    }

    pos = 0;
    while((pos = s.find("&nbsp;", pos)) != std::string::npos)
    {
        s.erase(pos, 6);
    }
}
