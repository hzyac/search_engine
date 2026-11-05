#include "runtimeConfig.hpp"
#include <arpa/inet.h>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>

namespace
{
std::string trim(const std::string &s)
{
    const auto first = s.find_first_not_of(" \t\r\n");
    return first == std::string::npos ? ""
                                      : s.substr(first, s.find_last_not_of(" \t\r\n") - first + 1);
}

unsigned int number(const std::string &s, unsigned int maximum, const std::string &name)
{
    if(s.empty() || s.find_first_not_of("0123456789") != std::string::npos)
    {
        throw std::runtime_error("Invalid " + name + ": " + s);
    }

    const auto n = std::stoul(s);
    if(n == 0 || n > maximum)
    {
        throw std::runtime_error("Invalid " + name + ": " + s);
    }

    return static_cast<unsigned int>(n);
}
}

void requireFile(const std::filesystem::path &file)
{
    std::ifstream input(file);
    if(!std::filesystem::is_regular_file(file) || !input)
    {
        throw std::runtime_error("Missing or unreadable file: " + file.string());
    }
}

void requireDirectory(const std::filesystem::path &directory)
{
    if(!std::filesystem::is_directory(directory))
    {
        throw std::runtime_error("Missing directory: " + directory.string());
    }
}

RuntimeConfig RuntimeConfig::load(const std::filesystem::path &file)
{
    requireFile(file);
    const auto base = std::filesystem::absolute(file).parent_path();
    std::ifstream input(file);
    std::map<std::string, std::string> values;

    const std::set<std::string> allowed = {
        "jieba_dir", "stop_words_eng", "stop_words_zh", "corpus_eng", "corpus_zh", "corpus_web",
        "data_dir",  "host",           "port",          "threads",    "queue_size"};

    std::string line;
    unsigned int lineNumber = 0;
    while(std::getline(input, line))
    {
        ++lineNumber;
        line = trim(line);
        if(line.empty() || line[0] == '#')
        {
            continue;
        }

        std::istringstream parser(line);
        std::string key, value;
        parser >> key;
        std::getline(parser, value);
        value = trim(value);
        if(!allowed.count(key) || value.empty() || !values.emplace(key, value).second)
        {
            throw std::runtime_error("Invalid/duplicate configuration at line " +
                                     std::to_string(lineNumber) + ": " + key);
        }
    }

    auto path = [&](const std::string &key) -> std::filesystem::path
    {
        const auto it = values.find(key);
        if(it == values.end())
        {
            return {};
        }

        return (base / std::filesystem::path(it->second)).lexically_normal();
    };

    RuntimeConfig config;
    config.jiebaDir = path("jieba_dir");
    config.stopWordsEn = path("stop_words_eng");
    config.stopWordsZh = path("stop_words_zh");
    config.corpusEn = path("corpus_eng");
    config.corpusZh = path("corpus_zh");
    config.corpusWeb = path("corpus_web");
    config.dataDir = path("data_dir");

    if(values.count("host"))
    {
        config.host = values.at("host");
    }

    if(values.count("port"))
    {
        config.port = parsePort(values.at("port"));
    }

    if(values.count("threads"))
    {
        config.threads = number(values.at("threads"), 256, "threads");
    }

    if(values.count("queue_size"))
    {
        config.queueSize = number(values.at("queue_size"), 1000000, "queue_size");
    }

    config.validateEndpoint();
    return config;
}

unsigned short parsePort(const std::string &value)
{
    return static_cast<unsigned short>(number(value, 65535, "port"));
}

void RuntimeConfig::validateEndpoint() const
{
    in_addr address{};
    if(inet_pton(AF_INET, host.c_str(), &address) != 1)
    {
        throw std::runtime_error("host must be an IPv4 address: " + host);
    }
}

void RuntimeConfig::validateResources() const
{
    requireDirectory(jiebaDir);
    for(const auto *name :
        {"jieba.dict.utf8", "hmm_model.utf8", "user.dict.utf8", "idf.utf8", "stop_words.utf8"})
    {
        requireFile(jiebaDir / name);
    }
}

std::string RuntimeConfig::dataFile(const std::string &name) const
{
    if(dataDir.empty())
    {
        throw std::runtime_error("Missing configuration: data_dir");
    }

    return (dataDir / name).string();
}

void RuntimeConfig::prepareOutput() const
{
    if(dataDir.empty())
    {
        throw std::runtime_error("Missing configuration: data_dir");
    }

    std::filesystem::create_directories(dataDir);
}

void RuntimeConfig::validateServerData() const
{
    validateResources();
    for(const auto *name :
        {"dict.dat", "dictIndex.dat", "newripepage.dat", "newoffset.dat", "invertIndex.dat"})
    {
        const auto file = dataFile(name);
        requireFile(file);
        if(std::filesystem::file_size(file) == 0)
        {
            throw std::runtime_error("Empty server data file: " + file);
        }
    }
}

std::optional<std::filesystem::path> defaultConfigFile()
{
    std::error_code error;
    const auto executable = std::filesystem::read_symlink("/proc/self/exe", error);
    if(error)
    {
        throw std::runtime_error("Cannot determine application path: " + error.message());
    }

    auto prefix = executable.parent_path().parent_path();
    for(int level = 0; level < 2; ++level, prefix = prefix.parent_path())
    {
        for(const auto &relative : {std::filesystem::path("conf/search.conf"),
                                    std::filesystem::path("etc/search_engine/search.conf")})
        {
            const auto candidate = prefix / relative;
            if(std::filesystem::is_regular_file(candidate))
            {
                return candidate;
            }
        }
    }

    return std::nullopt;
}

std::string configArgument(int argc, char **argv)
{
    const auto usage =
        "Usage: " + std::filesystem::path(argv[0]).filename().string() + " [--config <file>]";
    if(argc == 2 && std::string(argv[1]) == "--help")
    {
        std::cout << usage << "\n"
                  << "The default configuration is found relative to the executable.\n"
                  << "Resource paths are resolved from the configuration file.\n";
        return {};
    }

    if(argc == 1)
    {
        const auto file = defaultConfigFile();
        if(!file)
        {
            throw std::runtime_error("No default configuration found. " + usage);
        }

        return file->string();
    }

    if(argc != 3 || std::string(argv[1]) != "--config" || std::string(argv[2]).empty())
    {
        throw std::runtime_error(usage);
    }

    return argv[2];
}
