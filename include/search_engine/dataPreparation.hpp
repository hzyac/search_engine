#ifndef SEARCH_ENGINE_DATA_PREPARATION_HPP
#define SEARCH_ENGINE_DATA_PREPARATION_HPP

#include "document.hpp"
#include "runtimeConfig.hpp"

// 一个数据目录只允许一个写入者：在线服务或离线建库程序。
class DataLock
{
public:
    explicit DataLock(const std::filesystem::path &directory);
    ~DataLock();
    DataLock(const DataLock &) = delete;
    DataLock &operator=(const DataLock &) = delete;

private:
    int _fd = -1;
};

// 离线刷新原始语料，同时保留通过在线接口导入的文档。
void prepareData(const RuntimeConfig &config);

// 以下接口由持有 DataLock 的调用者使用。
RuntimeConfig ensurePreparedData(const RuntimeConfig &config);
RuntimeConfig buildDataVersion(const RuntimeConfig &config, const std::vector<Document> &documents,
                               const std::vector<Document> &imports,
                               const std::filesystem::path &seedDictionary);
void publishDataVersion(const RuntimeConfig &config, const RuntimeConfig &version);
void removeUnusedVersions(const RuntimeConfig &config, const RuntimeConfig &active);

#endif
