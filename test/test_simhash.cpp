#include <iostream>

// Suppress informational logs from the installed limonp version.
#define LOGGING_LEVEL limonp::LL_WARNING

#include <simhash/Simhasher.hpp>
#include "../include/dependencyPaths.hpp"
using namespace simhash;

int main(int argc, char** argv)
{
    // 初始化 simhash
    Simhasher simhasher(search_engine::dictPath("jieba.dict.utf8"),
                        search_engine::dictPath("hmm_model.utf8"),
                        search_engine::dictPath("idf.utf8"),
                        search_engine::dictPath("stop_words.utf8"));

    string s1("CSDN的文章: 我是蓝翔技工拖拉机学院手扶拖拉机专业的。不用多久，我就会升职加薪，当上总经理，出任CEO，走上人生巅峰。");
    uint64_t u1;       // 指纹1
    // 关键词, 权重
    vector<pair<string ,double> > res1;
    // 过滤、计算等
    simhasher.extract(s1, res1, 5); // 可以不写
    simhasher.make(s1, 5, u1); // 5个关键字

    // ==================== 第二个网页（几乎一样） ====================
    string s2("知乎的文章: 我是蓝翔技工拖拉机学院手扶拖拉机专业的。不用多久，我就会升职加薪，当上总经理，出任CEO，走上人生巅峰！");
    uint64_t u2;       // 指纹2
    // 关键词, 权重
    vector<pair<string ,double> > res2;
    // 过滤、计算等
    simhasher.extract(s2, res2, 5); // 可以不写
    simhasher.make(s2, 5, u2); // 5个关键字

    cout << "关键词序列是: " << res1 << endl;
    cout << "关键词序列是: " << res2<< endl;
    cout<< "海明距离阈值默认设置为3，则isEqual结果为：" << (Simhasher::isEqual(u1,u2)) << endl; 
    cout<< "海明距离阈值默认设置为5，则isEqual结果为：" << (Simhasher::isEqual(u1, u2, 5)) << endl; 
    return EXIT_SUCCESS;
}
