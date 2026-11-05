#ifndef SEARCH_ENGINE_RUNTIME_CONFIG_HPP
#define SEARCH_ENGINE_RUNTIME_CONFIG_HPP

#include <filesystem>
#include <string>
#include <optional>

struct RuntimeConfig
{
    std::filesystem::path jiebaDir;
    std::filesystem::path stopWordsEn;
    std::filesystem::path stopWordsZh;
    std::filesystem::path corpusEn;
    std::filesystem::path corpusZh;
    std::filesystem::path corpusWeb;
    std::filesystem::path dataDir;

    std::string host = "127.0.0.1";
    unsigned short port = 8888;
    unsigned int threads = 4;
    unsigned int queueSize = 64;

    static RuntimeConfig load(const std::filesystem::path &file);

    void validateEndpoint() const;
    void validateResources() const;
    void validateServerData() const;
    void prepareOutput() const;

    std::string dataFile(const std::string &name) const;
};

// 默认配置从可执行文件所在的构建目录或安装前缀查找，不依赖工作目录。
std::optional<std::filesystem::path> defaultConfigFile();

// 返回空字符串表示已输出 --help。
std::string configArgument(int argc, char **argv);

unsigned short parsePort(const std::string &value);
void requireFile(const std::filesystem::path &file);
void requireDirectory(const std::filesystem::path &directory);

#endif
