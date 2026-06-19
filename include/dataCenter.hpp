// DataCenter.hpp
#ifndef SEARCH_ENGINE_DATA_CENTER_HPP
#define SEARCH_ENGINE_DATA_CENTER_HPP

#include <vector>
#include <map>
#include <set>
#include <string>
#include <mutex>
#include <iostream>

using std::string;
using std::vector;
using std::map;
using std::set;
using std::pair;
using std::cout;
using std::endl;

class DataCenter {
public:
    static DataCenter *getInstance();

private:
    DataCenter() { }
    DataCenter(const DataCenter &other) = delete;
    DataCenter &operator=(const DataCenter &other) = delete;
    ~DataCenter()
    {
        cout << "~DataCenter()" << endl;
    }

public:
    vector<pair<string, int>> _dict;
    map<string, set<int>> _index;
    vector<pair<size_t, size_t>> _webPageOffset;
    map<string, set<pair<int, double>>> _invertedIndex;

private:
    static DataCenter *_pInstance;
    static std::once_flag _onceFlag;
};

#endif
