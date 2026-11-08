#include <cppjieba/Jieba.hpp>
#include "../include/dependencyPaths.hpp"

using namespace std;

const string DICT_PATH = search_engine::dictPath("jieba.dict.utf8");
const string HMM_PATH = search_engine::dictPath("hmm_model.utf8");
const string USER_DICT_PATH = search_engine::dictPath("user.dict.utf8");
const string IDF_PATH = search_engine::dictPath("idf.utf8");
const string STOP_WORD_PATH = search_engine::dictPath("stop_words.utf8");

int main(int argc, char** argv) 
{

    // 创建jieba对象, 固定的
    cppjieba::Jieba jieba(DICT_PATH,
        HMM_PATH,
        USER_DICT_PATH,
        IDF_PATH,
        STOP_WORD_PATH);

    vector<string>words;
    string sentence = "你好，我是一个中国人";
    // true是全切模式, false是精确模式, 只会返回合理的 
    jieba.Cut(sentence, words, true); 
    for(string &word : words)
    {
        cout << word << '\n';
    }
}
