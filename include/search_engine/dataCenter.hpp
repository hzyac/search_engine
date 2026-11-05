#ifndef SEARCH_ENGINE_DATA_CENTER_HPP
#define SEARCH_ENGINE_DATA_CENTER_HPP

#include "runtimeConfig.hpp"
#include <atomic>
#include <cstddef>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

// 一次查询持有一个完整版本，旧版本在最后一个读者退出后清理。
struct SearchData
{
    ~SearchData();

    RuntimeConfig _config;
    std::string _generation;
    std::vector<std::pair<std::string, int>> _dict;
    std::map<std::string, std::set<int>> _index;
    std::vector<std::pair<std::size_t, std::size_t>> _webPageOffset;
    std::map<std::string, std::set<std::pair<int, double>>> _invertedIndex;
    mutable std::atomic<bool> _retired{false};
};

class DataCenter
{
public:
    static DataCenter *getInstance();
    static std::shared_ptr<const SearchData> read(const RuntimeConfig &config);

    std::shared_ptr<const SearchData> snapshot() const;
    void publish(std::shared_ptr<const SearchData> data) noexcept;

private:
    DataCenter() = default;
    DataCenter(const DataCenter &) = delete;
    DataCenter &operator=(const DataCenter &) = delete;

    std::shared_ptr<const SearchData> _data;
};

#endif
