// SplitTool.hpp
#ifndef SEARCH_ENGINE_SPLIT_TOOL_HPP
#define SEARCH_ENGINE_SPLIT_TOOL_HPP

#include <vector>
#include <string>

using std::vector;
using std::string;

class SplitTool {
public:
    SplitTool();
    virtual ~SplitTool();
    virtual vector<string> cut(const string &sentence) = 0;
private:

};

#endif
