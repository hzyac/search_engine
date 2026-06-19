// dictProducer.cpp
#include "../include/dictProducer.hpp"
#include "../include/splitTool.hpp"
#include <dirent.h>
#include <cstring>
#include <iostream>
#include <fstream>
#include <sstream>
#include <iterator>

using std::cout;
using std::endl;
using std::cerr;
using std::ifstream;
using std::ofstream;
using std::istringstream;
using std::istreambuf_iterator;

// 构造函数
DictProducer::DictProducer(const string &dirPath)
{
    saveFiles(dirPath);
}

// 构造函数, 专门处理中文
DictProducer::DictProducer(const string &dirPath, SplitTool *SplitTool)
: _splitTool(SplitTool)
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
    map<string, int> wordCount;
    for (auto &filePath : _files)
    {
        ifstream ifs(filePath);
        if (!ifs)
        {
            cerr << "打开文件失败：" << filePath << endl;
            continue;
        }

        string line;
        while (getline(ifs, line))
        {
            // 转小写
            for (auto &ch : line)
            {
                ch = tolower(ch);
            }

            // 非字母 → 空格
            for (auto &ch : line)
            {
                if (!isalpha(ch))
                {
                    ch = ' ';
                }
            }

            // 分词并统计词频
            string word;
            istringstream iss(line);
            while (iss >> word)
            {
                ++wordCount[word]; 
            }
        }
        ifs.close();
    }

    // 2 清除掉停用词
    string filePath = "../corpus/stop_words_eng.txt"; 
    ifstream ifs(filePath);
    if (!ifs)
    {
        cerr << "打开文件失败：" << filePath << endl;
    }
    else
    {
        string word;
        while (ifs >> word)
        {
            if(wordCount.find(word) != wordCount.end())
            {
                wordCount.erase(word);
            }
        }
    }
    ifs.close();

    // 3 建立词典
    for (auto &elem : wordCount)
    {
        _dict.push_back(elem);
    }
}

// 创建中文字典
void DictProducer::buildCnDict()
{
    if (!_splitTool)
    {
        cerr << "中文分词工具未初始化！" << endl;
        return;
    }
    
    // 1 切割中文并统计数量
    map<string, int> wordCount;
    for (auto &filePath : _files)
    {
        ifstream ifs(filePath);
        if (!ifs)
        {
            cerr << "打开文件失败：" << filePath << endl;
            continue;
        }

        string content((istreambuf_iterator<char>(ifs)), istreambuf_iterator<char>());
        // 需要将'\n'全部去掉
        size_t pos = 0;
        while((pos = content.find('\n', pos)) != std::string::npos)
        {
            content.erase(pos, 1);
        }

        vector<string> words = _splitTool->cut(content);
        for (auto &word : words)
        {
            ++wordCount[word];  
        }
        ifs.close();
    }
    
    // 2 清除掉停用词
    string filePath = "../corpus/stop_words_zh.txt"; 
    ifstream ifs(filePath);
    if (!ifs)
    {
        cerr << "打开文件失败：" << filePath << endl;
    }
    else
    {
        string word;
        while (ifs >> word)
        {
            if(wordCount.find(word) != wordCount.end())
            {
                wordCount.erase(word);
            }
        }
    }
    ifs.close();

    // 3 建立词典
    for (auto &elem : wordCount)
    {
        _dict.push_back(elem);
    }
}

// 把字典写入文件
void DictProducer::storeDict(const char *filepath)
{
    ofstream ofs(filepath);
    if (!ofs)
    {
        cerr << "打开字典文件失败！" << endl;
        return;
    }

    for (auto &elem : _dict)
    {
        ofs << elem.first << " " << elem.second << endl;
    }

    ofs.close();
    cout << "词典文件保存成功：" << filepath << endl;
}

// 创建索引
void DictProducer::buildIndex()
{
    //记录下标, 写到文件中是第一行开始的 
    int i = 1;  
    for(auto elem : _dict)
    {
        string word = elem.first;
        // 单个字的数量
        size_t charNums = word.size() / getByteNum_UTF8(word[0]);
        // n 表示字, idx表示字符
        for(size_t idx = 0, n = 0; n != charNums; ++idx, ++n)
        {
            //按照字符个数切割
            size_t charLen = getByteNum_UTF8(word[idx]);
            //按照编码格式，进行拆分
            string subword = word.substr(idx, charLen);
            _index[subword].insert(i);
            idx += (charLen - 1);
        }
        ++i;
    }
}

// 将索引写入文件
void DictProducer::storeIndex(const char *filepath)
{
    ofstream ofs(filepath);
    if (!ofs)
    {
        cerr << "打开索引文件失败！" << endl;
        return;
    }

    for (auto &elem : _index)
    {
        ofs << elem.first << ' ';
        for(int index : elem.second)
        {
            ofs << index << ' ';
        }
        ofs << endl;
    }

    ofs.close();
    cout << "索引文件保存成功：" << filepath << endl;
}

// 保存目录里面的文件路径
void DictProducer::saveFiles(const string &dirPath)
{
    DIR *dir = opendir(dirPath.c_str());
    if (!dir)
    {
        cerr << "打开目录失败：" << dirPath << endl;
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

// 计算每个字占用几个字节
size_t DictProducer::getByteNum_UTF8(const char byte)
{
    int byteNum = 0;
    for (size_t i = 0; i < 6; ++i)
    {
        if (byte & (1 << (7 - i)))
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
size_t DictProducer::addDict(const DictProducer &other)
{
    size_t offset = _dict.size();
    for (auto &elem : other._dict)
    {
        _dict.push_back(elem);
    }
    return offset;
}

// 合并另一个DictProducer的索引数据，offset为索引偏移量
void DictProducer::addIndex(const DictProducer &other, size_t offset)
{
    for (auto &elem : other._index)
    {
        for (int idx : elem.second)
        {
            _index[elem.first].insert(idx + offset);
        }
    }
}