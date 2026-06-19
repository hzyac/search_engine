// createinvertedIndex.cpp
#include "../include/invertedIndex.hpp"

#include "../include/splitToolCppJieba.hpp"

int main()
{
    SplitToolCppJieba *split = new SplitToolCppJieba();
    InvertedIndex *invIdx = new InvertedIndex("../data/newripepage.dat",
                                                     "../data/newoffset.dat",
                                                     "../data/invertIndex.dat",
                                                     "../corpus/stop_words_eng.txt",
                                                     "../corpus/stop_words_zh.txt",
                                                     split);
    invIdx->buildInvertedIndex();
    invIdx->storeInvertedIndex();

    delete split;
    delete invIdx;

    return 0;
}