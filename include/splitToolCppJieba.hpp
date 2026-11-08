// SplitToolCppJieba.hpp
#pragma once

#include "splitTool.hpp"
#include <cppjieba/Jieba.hpp>

class SplitToolCppJieba
: public SplitTool
{
public:
    SplitToolCppJieba();
    virtual ~SplitToolCppJieba();

    virtual vector<string> cut(const string &sentence) override;
private:
    cppjieba::Jieba _jieba;
};
