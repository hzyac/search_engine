#include "cppjieba/Jieba.hpp"

using namespace std;

const char* const DICT_PATH = "./dict/jieba.dict.utf8";
const char* const HMM_PATH = "./dict/hmm_model.utf8";
const char* const USER_DICT_PATH = "./dict/user.dict.utf8";
const char* const IDF_PATH = "./dict/idf.utf8";
const char* const STOP_WORD_PATH = "./dict/stop_words.utf8";

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
