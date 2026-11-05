// dictProducer.cpp
#include "dictProducer.hpp"
#include <cctype>
#include "splitTool.hpp"
#include <dirent.h>
#include <algorithm>
#include <filesystem>
#include <stdexcept>
#include <cstring>
#include <iostream>
#include <fstream>
#include <sstream>
#include <iterator>

// 构造函数
DictProducer::DictProducer(const std::string &dirPath, const std::string &stopWords)
: _stopWords(stopWords)
{
    saveFiles(dirPath);
}

// 构造函数, 专门处理中文
DictProducer::DictProducer(const std::string &dirPath, const std::string &stopWords,
                           SplitTool *splitTool)
: _splitTool(splitTool)
, _stopWords(stopWords)
{
    saveFiles(dirPath);
}

// 析构函数
DictProducer::~DictProducer()
{
}

// 创建英文字典
void DictProducer::buildEnDict()
{
    // 1 英文单词清洗, 顺便统计词频
    std::map<std::string, int> wordCount;
    for(auto &filePath : _files)
    {
        std::ifstream ifs(filePath);
        if(!ifs)
        {
            std::cerr << "打开文件失败：" << filePath << std::endl;
            continue;
        }

        std::string line;
        while(std::getline(ifs, line))
        {
            // 转小写
            for(auto &ch : line)
            {
                ch = std::tolower(static_cast<unsigned char>(ch));
            }

            // 非字母 → 空格
            for(auto &ch : line)
            {
                if(!std::isalpha(static_cast<unsigned char>(ch)))
                {
                    ch = ' ';
                }
            }

            // 分词并统计词频
            std::string word;
            std::istringstream iss(line);
            while(iss >> word)
            {
                ++wordCount[word];
            }
        }

        ifs.close();
    }

    // 2 清除掉停用词
    std::string filePath = _stopWords;
    std::ifstream ifs(filePath);
    if(!ifs)
    {
        std::cerr << "打开文件失败：" << filePath << std::endl;
    }
    else
    {
        std::string word;
        while(ifs >> word)
        {
            if(wordCount.find(word) != wordCount.end())
            {
                wordCount.erase(word);
            }
        }
    }

    ifs.close();

    // 3 建立词典
    for(auto &elem : wordCount)
    {
        _dict.push_back(elem);
    }
}

// 创建中文字典
void DictProducer::buildCnDict()
{
    if(!_splitTool)
    {
        std::cerr << "中文分词工具未初始化！" << std::endl;
        return;
    }

    // 1 切割中文并统计数量
    std::map<std::string, int> wordCount;
    for(auto &filePath : _files)
    {
        std::ifstream ifs(filePath);
        if(!ifs)
        {
            std::cerr << "打开文件失败：" << filePath << std::endl;
            continue;
        }

        std::string content((std::istreambuf_iterator<char>(ifs)),
                            std::istreambuf_iterator<char>());

        // 需要将'\n'全部去掉
        std::size_t pos = 0;
        while((pos = content.find('\n', pos)) != std::string::npos)
        {
            content.erase(pos, 1);
        }

        std::vector<std::string> words = _splitTool->cut(content);
        for(auto &word : words)
        {
            if(!word.empty() && word.find_first_not_of(" \t\r\n") != std::string::npos)
            {
                ++wordCount[word];
            }
        }

        ifs.close();
    }

    // 2 清除掉停用词
    std::string filePath = _stopWords;
    std::ifstream ifs(filePath);
    if(!ifs)
    {
        std::cerr << "打开文件失败：" << filePath << std::endl;
    }
    else
    {
        std::string word;
        while(ifs >> word)
        {
            if(wordCount.find(word) != wordCount.end())
            {
                wordCount.erase(word);
            }
        }
    }

    ifs.close();

    // 3 建立词典
    for(auto &elem : wordCount)
    {
        _dict.push_back(elem);
    }
}

// 把字典写入文件
void DictProducer::storeDict(const char *filepath)
{
    std::ofstream ofs(filepath);
    if(!ofs)
    {
        throw std::runtime_error("Cannot write dictionary: " + std::string(filepath));
    }

    for(auto &elem : _dict)
    {
        ofs << elem.first << " " << elem.second << std::endl;
    }

    ofs.close();
    std::cout << "词典文件保存成功：" << filepath << std::endl;
}

// 创建索引
void DictProducer::buildIndex()
{
    //记录下标, 写到文件中是第一行开始的
    int i = 1;
    for(auto elem : _dict)
    {
        std::string word = elem.first;

        // 单个字的数量
        std::size_t charNums = word.size();

        // n 表示字, idx表示字符
        for(std::size_t idx = 0; idx < charNums; ++idx)
        {
            //按照字符个数切割
            std::size_t charLen = getByteNum_UTF8(word[idx]);

            //按照编码格式，进行拆分
            std::string subword = word.substr(idx, charLen);
            _index[subword].insert(i);
            idx += (charLen - 1);
        }

        ++i;
    }
}

// 将索引写入文件
void DictProducer::storeIndex(const char *filepath)
{
    std::ofstream ofs(filepath);
    if(!ofs)
    {
        throw std::runtime_error("Cannot write dictionary index: " + std::string(filepath));
    }

    for(auto &elem : _index)
    {
        ofs << elem.first << ' ';
        for(int index : elem.second)
        {
            ofs << index << ' ';
        }

        ofs << std::endl;
    }

    ofs.close();
    std::cout << "索引文件保存成功：" << filepath << std::endl;
}

// 保存目录里面的文件路径
void DictProducer::saveFiles(const std::string &dirPath)
{
    DIR *dir = opendir(dirPath.c_str());
    if(!dir)
    {
        std::cerr << "打开目录失败：" << dirPath << std::endl;
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

// 计算每个字占用几个字节
std::size_t DictProducer::getByteNum_UTF8(const char byte)
{
    int byteNum = 0;
    for(std::size_t i = 0; i < 6; ++i)
    {
        if(byte & (1 << (7 - i)))
        {
            ++byteNum;
        }
        else
        {
            break;
        }
    }

    return byteNum == 0 ? 1 : byteNum;
}

// 合并另一个DictProducer的词典数据，返回合并前本词典的大小（用于索引偏移）
std::size_t DictProducer::addDict(const DictProducer &other)
{
    std::size_t offset = _dict.size();
    for(auto &elem : other._dict)
    {
        _dict.push_back(elem);
    }

    return offset;
}

// 合并另一个DictProducer的索引数据，offset为索引偏移量
void DictProducer::addIndex(const DictProducer &other, std::size_t offset)
{
    for(auto &elem : other._index)
    {
        for(int idx : elem.second)
        {
            _index[elem.first].insert(idx + offset);
        }
    }
}
