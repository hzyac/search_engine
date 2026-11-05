// SplitTool.hpp
#ifndef SEARCH_ENGINE_SPLIT_TOOL_HPP
#define SEARCH_ENGINE_SPLIT_TOOL_HPP

#include <vector>
#include <string>

class SplitTool
{
public:
    SplitTool();
    virtual ~SplitTool();
    virtual std::vector<std::string> cut(const std::string &sentence) = 0;

private:
};

#endif
