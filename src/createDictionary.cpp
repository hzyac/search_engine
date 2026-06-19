// createDictionary.cpp
#include "../include/splitToolCppJieba.hpp"
#include "../include/dictProducer.hpp"

int main()
{
    // 创建jieba对象
    SplitToolCppJieba cppjieba;

    // 创建英文词典类
    DictProducer dict1("../corpus/eng");
    // 创建英文字典
    dict1.buildEnDict();
    // 创建英文索引
    dict1.buildIndex();

    // 创建中文词典类
    DictProducer dict2("../corpus/art", &cppjieba);
    // 创建中文字典
    dict2.buildCnDict();
    // 创建中文索引
    dict2.buildIndex();

    // 将英文词典和索引合并到中文词典中
    size_t offset = dict2.addDict(dict1);
    dict2.addIndex(dict1, offset);

    // 合并后统一写入文件
    dict2.storeDict("../data/dict.dat");
    dict2.storeIndex("../data/dictIndex.dat");

    return 0;
}
