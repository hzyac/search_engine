#include "../include/dependencyPaths.hpp"
#include "../include/splitToolCppJieba.hpp"
#include <nlohmann/json.hpp>
#include <simhash/Simhasher.hpp>
#include <algorithm>
#include <iostream>
#include <numeric>
#include <stdexcept>

int main()
{
    try {
        const std::string text = "韩玉鉴赏在中国学习自然语言处理";
        SplitToolCppJieba splitter;
        const auto words = splitter.cut(text);
        if (std::accumulate(words.begin(), words.end(), std::string()) != text
            || std::find(words.begin(), words.end(), "韩玉鉴赏") == words.end()) {
            throw std::runtime_error("分词结果不完整，或项目自定义词典未加载");
        }

        simhash::Simhasher hasher(search_engine::dictPath("jieba.dict.utf8"),
                                 search_engine::dictPath("hmm_model.utf8"),
                                 search_engine::dictPath("idf.utf8"),
                                 search_engine::dictPath("stop_words.utf8"));
        uint64_t first = 0, second = 0;
        std::vector<std::pair<std::string, double>> keywords;
        if (!hasher.extract(text, keywords, 5) || keywords.empty()
            || !hasher.make(text, 5, first) || !hasher.make(text, 5, second)
            || first != second || !simhash::Simhasher::isEqual(first, second)
            || simhash::Simhasher::isEqual(first, first ^ 0xFULL, 3)) {
            throw std::runtime_error("SimHash 关键词抽取、指纹或海明距离检查失败");
        }

        const nlohmann::json document = {{"words", words}, {"fingerprint", first}};
        const auto parsed = nlohmann::json::parse(document.dump());
        if (parsed.at("words").get<std::vector<std::string>>() != words
            || parsed.at("fingerprint").get<uint64_t>() != first) {
            throw std::runtime_error("JSON 中文分词和 64 位指纹往返检查失败");
        }
        std::cout << "依赖检查通过：中文分词、自定义词典、SimHash、JSON" << std::endl;
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << std::endl;
        return 1;
    }
}
