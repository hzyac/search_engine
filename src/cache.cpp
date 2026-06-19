// Cache.cpp
#include "../include/cache.hpp"
#include <ctime>
#include <iostream>

using std::cout;
using std::endl;

// 设置缓存容量，默认为10
Cache::Cache(size_t capacity)
: _capacity(capacity)
{
    cout << "Cache() 构造, capacity: " << _capacity << endl;
}

// 析构函数，输出剩余缓存项数量
Cache::~Cache()
{
    cout << "~Cache() 析构, 剩余缓存项: " << _dataList.size() << endl;
}

// 用于缓存同步时上锁
void Cache::lock()
{
    _mutex.lock();
}

// 解锁
void Cache::unlock()
{
    _mutex.unlock();
}

// 查找是否存在，存在则返回对应值，否则返回空字符串
string Cache::get(const string &key)
{
    auto it = _cacheMap.find(key);
    if(it != _cacheMap.end())
    {
        _dataList.splice(_dataList.begin(), _dataList, it->second);
        return it->second->second;
    }
    return string();
}

// 设置缓存项，如果已存在则更新值并移动到前面，如果不存在则添加新项，超过容量则移除最旧项
void Cache::set(const string &key, const string &value)
{
    auto it = _cacheMap.find(key);
    if(it != _cacheMap.end())
    {
        it->second->second = value;
        _dataList.splice(_dataList.begin(), _dataList, it->second);
        return;
    }

    if(_dataList.size() >= _capacity)
    {
        string lastKey = _dataList.back().first;
        _cacheMap.erase(lastKey);
        _dataList.pop_back();
    }

    _dataList.push_front({key, value});
    _cacheMap[key] = _dataList.begin();
}

// 设置线程缓存同步时的待处理项
void Cache::setPending(const string &key, const string &value)
{
    _pendingDataMap[time(nullptr)] = {key, value};
}

// 返回当前缓存项数量
size_t Cache::cache_size()
{
    return _dataList.size();
}

// 清空待更新项
void Cache::clear()
{
    _pendingDataMap.clear();
}

// 主缓存功能: 合并线程缓存的更新内容到主缓存
void Cache::merge(const Cache &subcache)
{
    auto it = subcache._pendingDataMap.begin();
    while(it != subcache._pendingDataMap.end())
    {
        set(it->second.first, it->second.second);
        ++it;
    }
}

// 子缓存功能: 更新线程缓存的内容, 使得和主缓存保持一致
void Cache::update(const Cache &maincache)
{
    _cacheMap.clear();
    _dataList.clear();
    for(const auto &pair : maincache._dataList)
    {
        set(pair.first, pair.second);
    }
}