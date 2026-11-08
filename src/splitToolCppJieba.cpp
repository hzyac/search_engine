// SplitToolCppJieba.cpp
#include "../include/splitToolCppJieba.hpp"
#include "../include/dependencyPaths.hpp"
#include <iostream>

using std::cout;
using std::endl;

SplitToolCppJieba::SplitToolCppJieba()
: _jieba(search_engine::dictPath("jieba.dict.utf8"),
         search_engine::dictPath("hmm_model.utf8"),
         search_engine::dictPath("user.dict.utf8"),
         search_engine::dictPath("idf.utf8"),
         search_engine::dictPath("stop_words.utf8"))
{
    cout << "SplitToolCppJieba init success" << endl;
}

SplitToolCppJieba::~SplitToolCppJieba()
{

}

vector<string> SplitToolCppJieba::cut(const string &sentence)
{
    vector<string> words;
    _jieba.Cut(sentence, words);
    return words;
}
