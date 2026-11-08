// dictProducer.hpp
#pragma once

#include <vector>
#include <string>
#include <map>
#include <set>

using std::vector;
using std::string;
using std::pair;
using std::map;
using std::set;

class SplitTool;

// 词典创建类
class DictProducer {
public:
    // 构造函数
    DictProducer(const string &dirPath);
    // 构造函数, 专门为中文处理
    DictProducer(const string &dirPath, SplitTool *SplitTool);
    // 析构函数
    ~DictProducer();
    // 创建英文字典
    void buildEnDict();
    // 创建中文字典
    void buildCnDict();
    // 将词典写入文件
    void storeDict(const char *filepath);
    // 创建索引
    void buildIndex();
    // 将索引写入文件
    void storeIndex(const char *filepath);
    // 合并另一个DictProducer的词典数据，返回合并前本词典的大小（用于索引偏移）
    size_t addDict(const DictProducer &other);
    // 合并另一个DictProducer的索引数据，offset为索引偏移量
    void addIndex(const DictProducer &other, size_t offset);

private:
    // 保存目录里面的文件路径
    void saveFiles(const string &dirPath);
    // 计算每个字占用几个字节
    size_t getByteNum_UTF8(const char byte);

private:
    // 语料库文件的绝对路径的集合
    vector<string> _files;
    // 词典
    vector<pair<string, int>> _dict;
    // 分词工具
    SplitTool *_splitTool;
    // 词典索引
    map<string, set<int>> _index;
};
