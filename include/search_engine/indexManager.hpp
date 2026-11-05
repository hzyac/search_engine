#ifndef SEARCH_ENGINE_INDEX_MANAGER_HPP
#define SEARCH_ENGINE_INDEX_MANAGER_HPP

#include "dataPreparation.hpp"
#include <nlohmann/json.hpp>
#include <condition_variable>
#include <cstdint>
#include <map>
#include <mutex>
#include <optional>
#include <thread>

// 单个后台写入者构建新版本，查询线程始终读取已发布快照。
class IndexManager
{
public:
    explicit IndexManager(const RuntimeConfig &config);
    ~IndexManager();

    nlohmann::json submit(const nlohmann::json &request);
    nlohmann::json status(const nlohmann::json &request) const;

private:
    struct Import
    {
        std::uint64_t id;
        std::vector<Document> documents;
    };

    void work();
    void recover();
    void remember(nlohmann::json status);

    RuntimeConfig _config;
    DataLock _lock;
    mutable std::mutex _mutex;
    std::condition_variable _condition;
    std::optional<Import> _pending;
    nlohmann::json _status;
    std::map<std::uint64_t, nlohmann::json> _history;
    std::uint64_t _nextId;
    bool _stopping = false;
    std::thread _worker;
};

#endif
