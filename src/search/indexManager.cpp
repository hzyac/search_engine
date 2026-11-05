#include "indexManager.hpp"
#include "dataCenter.hpp"
#include <algorithm>
#include <chrono>
#include <fstream>
#include <iostream>
#include <set>
#include <stdexcept>

namespace
{
std::vector<Document> parseBatch(const nlohmann::json &value)
{
    if(!value.is_array() || value.empty())
    {
        throw std::runtime_error("documents must be a non-empty array");
    }

    std::vector<Document> documents;
    std::set<std::string> ids;
    for(const auto &item : value)
    {
        auto document = Document::parse(item);
        if(!ids.insert(document.id).second)
        {
            throw std::runtime_error("Duplicate document id in batch: " + document.id);
        }

        documents.push_back(std::move(document));
    }

    return documents;
}

nlohmann::json record(std::uint64_t id, const std::vector<Document> &documents)
{
    auto values = nlohmann::json::array();
    for(const auto &document : documents)
    {
        values.push_back(document.json());
    }

    return {{"task_id", id}, {"status", "queued"}, {"documents", std::move(values)}};
}
}

IndexManager::IndexManager(const RuntimeConfig &config)
: _config(config)
, _lock(config.dataDir)
, _nextId(static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::microseconds>(
                                         std::chrono::system_clock::now().time_since_epoch())
                                         .count()))
{
    const auto active = ensurePreparedData(config);
    const auto data = DataCenter::read(active);
    DataCenter::getInstance()->publish(data);
    removeUnusedVersions(config, active);

    _status = {{"status", "idle"},
               {"generation", data->_generation},
               {"document_count", data->_webPageOffset.size() - 1}};
    if(std::filesystem::exists(active.dataFile("task.json")))
    {
        std::ifstream input(active.dataFile("task.json"));
        _status = nlohmann::json::parse(input);
        _nextId = std::max(_nextId, _status.at("task_id").get<std::uint64_t>());
    }

    remember(_status);
    recover();
    remember(_status);
    _worker = std::thread(&IndexManager::work, this);
}

IndexManager::~IndexManager()
{
    {
        std::lock_guard<std::mutex> lock(_mutex);
        _stopping = true;
    }

    _condition.notify_one();
    _worker.join();
}

void IndexManager::recover()
{
    const auto pending = _config.dataFile("pending-import.json");
    if(!std::filesystem::exists(pending))
    {
        return;
    }

    std::ifstream input(pending);
    const auto saved = nlohmann::json::parse(input);
    const auto id = saved.at("task_id").get<std::uint64_t>();
    _nextId = std::max(_nextId, id);
    if(_status.value("status", "") == "succeeded" &&
       _status.value("task_id", std::uint64_t{0}) == id)
    {
        std::filesystem::remove(pending);
        return;
    }

    _status["task_id"] = id;
    _status["received_documents"] = saved.at("documents").size();
    if(saved.value("status", "") == "failed")
    {
        _status["status"] = "failed";
        _status["error"] = saved.at("error");
        return;
    }

    _pending = Import{id, parseBatch(saved.at("documents"))};
    _status["status"] = "queued";
}

nlohmann::json IndexManager::submit(const nlohmann::json &request)
{
    if(!request.is_object() || !request.contains("documents"))
    {
        throw std::runtime_error("Import request requires a documents array");
    }

    auto documents = parseBatch(request.at("documents"));
    std::lock_guard<std::mutex> lock(_mutex);
    if(_stopping || _pending || _status.value("status", "") == "processing")
    {
        throw std::runtime_error("An import is already in progress; wait for it to finish");
    }

    const auto id = ++_nextId;
    auto queued = _status;
    queued["task_id"] = id;
    queued["status"] = "queued";
    queued.erase("error");
    queued["received_documents"] = documents.size();

    // 先记录请求，再返回接收成功；进程中断后会重新执行未发布的导入。
    writeJson(_config.dataFile("pending-import.json"), record(id, documents));
    _pending = Import{id, std::move(documents)};
    remember(std::move(queued));
    _condition.notify_one();
    return _status;
}

nlohmann::json IndexManager::status(const nlohmann::json &request) const
{
    if(!request.is_object())
    {
        throw std::runtime_error("Status request must be a JSON object");
    }

    std::lock_guard<std::mutex> lock(_mutex);
    if(request.contains("task_id"))
    {
        if(!request.at("task_id").is_number_unsigned())
        {
            throw std::runtime_error("task_id must be an unsigned integer");
        }

        const auto found = _history.find(request.at("task_id").get<std::uint64_t>());
        if(found == _history.end())
        {
            throw std::runtime_error("Unknown task id; the latest 32 tasks are retained");
        }

        return found->second;
    }

    return _status;
}

// 调用方持有 _mutex；构造期间尚未启动工作线程。
void IndexManager::remember(nlohmann::json status)
{
    _status = std::move(status);
    if(_status.contains("task_id"))
    {
        _history[_status.at("task_id").get<std::uint64_t>()] = _status;
        while(_history.size() > 32)
        {
            _history.erase(_history.begin());
        }
    }
}

void IndexManager::work()
{
    while(true)
    {
        Import job;
        {
            std::unique_lock<std::mutex> lock(_mutex);
            _condition.wait(lock,
                            [this]
                            {
                                return _stopping || _pending.has_value();
                            });
            if(!_pending)
            {
                return;
            }

            job = std::move(*_pending);
            _pending.reset();
            _status["status"] = "processing";
            remember(_status);
        }

        std::optional<RuntimeConfig> next;
        bool published = false;
        try
        {
            const auto previous = DataCenter::getInstance()->snapshot();
            const auto &active = previous->_config;
            const auto documents =
                mergeDocuments(readDocuments(active.dataFile("documents.jsonl")), job.documents);
            const auto imports =
                mergeDocuments(readDocuments(active.dataFile("imports.jsonl")), job.documents);
            next = buildDataVersion(_config, documents, imports, active.dataFile("seed.dict"));
            const auto data = DataCenter::read(*next);
            nlohmann::json completed = {{"task_id", job.id},
                                        {"status", "succeeded"},
                                        {"received_documents", job.documents.size()},
                                        {"document_count", documents.size()},
                                        {"generation", data->_generation}};
            writeJson(next->dataFile("task.json"), completed);

            publishDataVersion(_config, *next);
            published = true;
            DataCenter::getInstance()->publish(data);
            {
                std::lock_guard<std::mutex> lock(_mutex);
                // succeeded 表示新请求已能读到新索引。
                remember(std::move(completed));
                std::error_code error;
                std::filesystem::remove(_config.dataFile("pending-import.json"), error);
            }
        }
        catch(const std::exception &error)
        {
            if(next && !published)
            {
                std::error_code ignored;
                std::filesystem::remove_all(next->dataDir, ignored);
            }

            auto failed = record(job.id, job.documents);
            failed["status"] = "failed";
            failed["error"] = error.what();
            std::lock_guard<std::mutex> lock(_mutex);
            _status["status"] = "failed";
            _status["error"] = error.what();
            remember(_status);
            try
            {
                writeJson(_config.dataFile("pending-import.json"), failed);
            }
            catch(const std::exception &savingError)
            {
                std::cerr << "Cannot save import failure: " << savingError.what() << '\n';
            }
        }
    }
}
