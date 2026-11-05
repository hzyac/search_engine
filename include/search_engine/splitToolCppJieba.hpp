// SplitToolCppJieba.hpp
#ifndef SEARCH_ENGINE_SPLIT_TOOL_CPPJIEBA_HPP
#define SEARCH_ENGINE_SPLIT_TOOL_CPPJIEBA_HPP

#include "splitTool.hpp"
#include "runtimeConfig.hpp"
#include <cppjieba/Jieba.hpp>

class SplitToolCppJieba : public SplitTool
{
public:
    explicit SplitToolCppJieba(const RuntimeConfig &config);
    ~SplitToolCppJieba() override;

    virtual std::vector<std::string> cut(const std::string &sentence) override;

private:
    cppjieba::Jieba _jieba;
};

#endif
