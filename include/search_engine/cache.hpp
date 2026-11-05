// Cache.hpp
#ifndef SEARCH_ENGINE_CACHE_HPP
#define SEARCH_ENGINE_CACHE_HPP

#include <string>
#include <cstddef>
#include <list>
#include <mutex>
#include <unordered_map>
#include <map>

class Cache
{
public:
    // 设置缓存容量，默认为10
    Cache(std::size_t capacity = 10);

    // 析构函数，输出剩余缓存项数量
    ~Cache();

    // 用于缓存同步时上锁
    void lock();

    // 解锁
    void unlock();

    // 查找是否存在，存在则返回对应值，否则返回空字符串
    std::string get(const std::string &key);

    // 设置缓存项，如果已存在则更新值并移动到前面，如果不存在则添加新项，超过容量则移除最旧项
    void set(const std::string &key, const std::string &value);

    // 设置线程缓存同步时的待处理项
    void setPending(const std::string &key, const std::string &value);

    // 返回当前缓存项数量
    std::size_t cache_size();

    // 清空待更新项
    void clear();

    // 主缓存功能: 合并线程缓存的更新内容到主缓存
    void merge(const Cache &subcache);

    // 子缓存功能: 更新线程缓存的内容, 使得和主缓存保持一致
    void update(const Cache &maincache);

private:
    std::unordered_map<std::string, std::list<std::pair<std::string, std::string>>::iterator>
        _cacheMap;
    std::list<std::pair<std::string, std::string>> _dataList;
    std::mutex _mutex;
    std::size_t _capacity;
    std::map<std::string, std::string> _pendingDataMap;
};

#endif
