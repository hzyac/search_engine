#include "splitToolCppJieba.hpp"

SplitToolCppJieba::SplitToolCppJieba(const RuntimeConfig &config)
: _jieba((config.jiebaDir / "jieba.dict.utf8").string(),
         (config.jiebaDir / "hmm_model.utf8").string(),
         (config.jiebaDir / "user.dict.utf8").string(), (config.jiebaDir / "idf.utf8").string(),
         (config.jiebaDir / "stop_words.utf8").string())
{
}

SplitToolCppJieba::~SplitToolCppJieba() = default;

std::vector<std::string> SplitToolCppJieba::cut(const std::string &sentence)
{
    std::vector<std::string> words;
    _jieba.Cut(sentence, words, true);
    return words;
}
