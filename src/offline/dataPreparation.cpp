#include "dataPreparation.hpp"
#include "dictProducer.hpp"
#include "invertedIndex.hpp"
#include "splitToolCppJieba.hpp"
#include "webPage.hpp"
#include <nlohmann/json.hpp>
#include <cctype>
#include <map>
#include <set>

#include <sys/file.h>
#include <fcntl.h>
#include <unistd.h>
#include <cerrno>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iterator>
#include <stdexcept>
#include <iostream>
#include <sstream>
#include <system_error>
#include <utility>
#include <vector>

namespace
{
constexpr const char *files[] = {"dict.dat", "dictIndex.dat", "newripepage.dat", "newoffset.dat",
                                 "invertIndex.dat"};

std::string signature(const RuntimeConfig &config)
{
    std::ostringstream output;
    output << "search-engine-data-v1\n";
    for(const auto &path : {config.jiebaDir, config.stopWordsEn, config.stopWordsZh,
                            config.corpusEn, config.corpusZh, config.corpusWeb})
    {
        output << std::quoted(path.string()) << '\n';
    }

    return output.str();
}

bool ready(const RuntimeConfig &config)
{
    for(const auto *name : files)
    {
        const auto path = config.dataFile(name);
        if(!std::filesystem::is_regular_file(path) || std::filesystem::file_size(path) == 0)
        {
            return false;
        }
    }

    std::ifstream manifest(config.dataFile("data.ready"));
    return manifest &&
           std::string(std::istreambuf_iterator<char>(manifest), {}) == signature(config);
}

struct StagingDirectory
{
    std::filesystem::path path;
    bool keep = false;

    explicit StagingDirectory(const std::filesystem::path &directory,
                              const std::string &prefix = ".prepare-")
    {
        std::filesystem::create_directories(directory);
        const auto pattern = (directory / (prefix + "XXXXXX")).string();
        std::vector<char> buffer(pattern.begin(), pattern.end());
        buffer.push_back('\0');

        auto result = ::mkdtemp(buffer.data());
        if(!result)
        {
            throw std::system_error(errno, std::generic_category(), "create staging directory");
        }

        path = result;
    }

    ~StagingDirectory()
    {
        std::error_code error;
        if(!keep)
        {
            std::filesystem::remove_all(path, error);
        }
    }
};

void buildLegacyData(const RuntimeConfig &config)
{
    config.validateResources();
    requireDirectory(config.corpusEn);
    requireDirectory(config.corpusZh);
    requireDirectory(config.corpusWeb);
    requireFile(config.stopWordsEn);
    requireFile(config.stopWordsZh);

    StagingDirectory stage(config.dataDir);
    RuntimeConfig output = config;
    output.dataDir = stage.path;

    SplitToolCppJieba tokenizer(config);

    std::cout << "[1/3] Generating recommendation dictionary..." << std::endl;
    DictProducer english(config.corpusEn.string(), config.stopWordsEn.string());
    english.buildEnDict();
    english.buildIndex();

    DictProducer chinese(config.corpusZh.string(), config.stopWordsZh.string(), &tokenizer);
    chinese.buildCnDict();
    chinese.buildIndex();

    const auto offset = chinese.addDict(english);
    chinese.addIndex(english, offset);
    chinese.storeDict(output.dataFile("dict.dat").c_str());
    chinese.storeIndex(output.dataFile("dictIndex.dat").c_str());

    std::cout << "[2/3] Generating deduplicated webpage library..." << std::endl;
    WebPage pages(config.corpusWeb.string(), config);
    pages.buildPage(output.dataFile("newripepage.dat"), output.dataFile("newoffset.dat"));

    std::cout << "[3/3] Generating inverted index..." << std::endl;
    InvertedIndex index(output.dataFile("newripepage.dat"), output.dataFile("newoffset.dat"),
                        output.dataFile("invertIndex.dat"), config.stopWordsEn.string(),
                        config.stopWordsZh.string(), &tokenizer);
    index.buildInvertedIndex();
    index.storeInvertedIndex();
    output.validateServerData();

    // 写入期间无在线读者。最后发布完成标记，中断后不会加载半套数据。
    const auto marker = config.dataFile("data.ready");
    std::filesystem::remove(marker);
    for(const auto *name : files)
    {
        std::filesystem::rename(output.dataFile(name), config.dataFile(name));
    }

    const auto temporary = config.dataFile("data.ready.tmp");
    std::ofstream manifest(temporary);
    manifest.exceptions(std::ios::badbit | std::ios::failbit);
    manifest << signature(config);
    manifest.close();

    std::filesystem::rename(temporary, marker);

    std::cout << "Data preparation complete: " << config.dataDir << std::endl;
}
}

namespace
{
RuntimeConfig currentVersion(const RuntimeConfig &config)
{
    std::ifstream input(config.dataFile("CURRENT"));
    const auto name = nlohmann::json::parse(input).at("version").get<std::string>();
    if(name.rfind("index-", 0) != 0 || std::filesystem::path(name).filename() != name)
    {
        throw std::runtime_error("Invalid index version in CURRENT");
    }

    auto version = config;
    version.dataDir = config.dataDir / "versions" / name;
    requireFile(version.dataFile("documents.jsonl"));
    requireFile(version.dataFile("imports.jsonl"));
    requireFile(version.dataFile("seed.dict"));
    return version;
}

std::vector<Document> legacyDocuments(const RuntimeConfig &config)
{
    std::ifstream offsets(config.dataFile("newoffset.dat"));
    std::ifstream pages(config.dataFile("newripepage.dat"));
    std::vector<Document> documents;
    int id;
    std::size_t position, length;
    while(offsets >> id >> position >> length)
    {
        pages.seekg(position);
        std::string page(length, '\0');
        if(!pages.read(page.data(), length))
        {
            throw std::runtime_error("Cannot read legacy webpage");
        }

        auto field = [&](const std::string &name)
        {
            const auto opening = "<" + name + ">";
            const auto start = page.find(opening);
            const auto end = page.find("</" + name + ">", start);
            if(start == std::string::npos || end == std::string::npos)
            {
                throw std::runtime_error("Invalid legacy webpage field: " + name);
            }

            return page.substr(start + opening.size(), end - start - opening.size());
        };

        auto url = field("url");
        documents.push_back({url, field("title"), url, field("content")});
    }

    return mergeDocuments({}, documents);
}

void buildDictionary(const RuntimeConfig &output, const std::vector<Document> &documents,
                     SplitTool &tokenizer)
{
    std::map<std::string, int> words;
    std::ifstream seed(output.dataFile("seed.dict"));
    std::string word;
    int count;
    while(seed >> word >> count)
    {
        words[word] += count;
    }

    std::set<std::string> stopWords;
    for(const auto &file : {output.stopWordsEn, output.stopWordsZh})
    {
        requireFile(file);
        std::ifstream input(file);
        while(input >> word)
        {
            stopWords.insert(word);
        }
    }

    for(const auto &document : documents)
    {
        for(const auto &text : {document.title, document.content})
        {
            for(auto token : tokenizer.cut(text))
            {
                for(auto &ch : token)
                {
                    ch = std::tolower(static_cast<unsigned char>(ch));
                }

                if(!token.empty() && token.find_first_of(" \t\r\n") == std::string::npos &&
                   !stopWords.count(token))
                {
                    ++words[token];
                }
            }
        }
    }

    std::ofstream dictionary(output.dataFile("dict.dat"));
    dictionary.exceptions(std::ios::badbit | std::ios::failbit);
    std::map<std::string, std::set<int>> characters;
    int id = 0;
    for(const auto &entry : words)
    {
        dictionary << entry.first << ' ' << entry.second << '\n';
        ++id;
        for(std::size_t position = 0; position < entry.first.size();)
        {
            const auto byte = static_cast<unsigned char>(entry.first[position]);
            const std::size_t length = byte < 0x80 ? 1 : byte < 0xE0 ? 2 : byte < 0xF0 ? 3 : 4;
            characters[entry.first.substr(position, length)].insert(id);
            position += length;
        }
    }
    dictionary.close();

    std::ofstream index(output.dataFile("dictIndex.dat"));
    index.exceptions(std::ios::badbit | std::ios::failbit);
    for(const auto &entry : characters)
    {
        index << entry.first;
        for(int wordId : entry.second)
        {
            index << ' ' << wordId;
        }
        index << '\n';
    }
    index.close();
}
}

DataLock::DataLock(const std::filesystem::path &directory)
{
    if(directory.empty())
    {
        throw std::runtime_error("Missing configuration: data_dir");
    }

    std::filesystem::create_directories(directory);
    _fd = ::open((directory / ".data.lock").c_str(), O_CREAT | O_RDWR | O_CLOEXEC, 0644);
    if(_fd < 0)
    {
        throw std::system_error(errno, std::generic_category(), "open data lock");
    }

    if(::flock(_fd, LOCK_EX | LOCK_NB) < 0)
    {
        const int error = errno;
        ::close(_fd);
        _fd = -1;
        throw std::system_error(
            error, std::generic_category(),
            "Data directory is in use by another server or preparation process");
    }
}

DataLock::~DataLock()
{
    if(_fd >= 0)
    {
        ::close(_fd);
    }
}

RuntimeConfig buildDataVersion(const RuntimeConfig &config, const std::vector<Document> &documents,
                               const std::vector<Document> &imports,
                               const std::filesystem::path &seedDictionary)
{
    if(documents.empty())
    {
        throw std::runtime_error("Cannot build an empty document collection");
    }

    config.validateResources();
    StagingDirectory stage(config.dataDir / "versions", "index-");
    auto output = config;
    output.dataDir = stage.path;
    std::filesystem::copy_file(seedDictionary, output.dataFile("seed.dict"));
    writeDocuments(output.dataFile("documents.jsonl"), documents);
    writeDocuments(output.dataFile("imports.jsonl"), imports);

    std::ofstream pages(output.dataFile("newripepage.dat"), std::ios::binary);
    std::ofstream offsets(output.dataFile("newoffset.dat"));
    pages.exceptions(std::ios::badbit | std::ios::failbit);
    offsets.exceptions(std::ios::badbit | std::ios::failbit);
    int id = 0;
    for(const auto &document : documents)
    {
        const auto page = document.page();
        offsets << ++id << ' ' << pages.tellp() << ' ' << page.size() << '\n';
        pages << page;
    }
    pages.close();
    offsets.close();

    SplitToolCppJieba tokenizer(config);
    buildDictionary(output, documents, tokenizer);
    InvertedIndex index(output.dataFile("newripepage.dat"), output.dataFile("newoffset.dat"),
                        output.dataFile("invertIndex.dat"), config.stopWordsEn.string(),
                        config.stopWordsZh.string(), &tokenizer);
    index.buildInvertedIndex(documents);
    index.storeInvertedIndex();
    output.validateServerData();

    std::ofstream marker(output.dataFile("data.ready"));
    marker.exceptions(std::ios::badbit | std::ios::failbit);
    marker << signature(config);
    marker.close();
    stage.keep = true;
    return output;
}

void publishDataVersion(const RuntimeConfig &config, const RuntimeConfig &version)
{
    writeJson(config.dataFile("CURRENT"), {{"version", version.dataDir.filename().string()}});
}

void removeUnusedVersions(const RuntimeConfig &config, const RuntimeConfig &active)
{
    for(const auto &entry : std::filesystem::directory_iterator(config.dataDir / "versions"))
    {
        if(entry.path() != active.dataDir &&
           entry.path().filename().string().rfind("index-", 0) == 0)
        {
            std::error_code error;
            std::filesystem::remove_all(entry.path(), error);
        }
    }
}

void prepareData(const RuntimeConfig &config)
{
    DataLock lock(config.dataDir);
    std::vector<Document> imports;
    if(std::filesystem::exists(config.dataFile("CURRENT")))
    {
        imports = readDocuments(currentVersion(config).dataFile("imports.jsonl"));
    }

    StagingDirectory stage(config.dataDir);
    auto legacy = config;
    legacy.dataDir = stage.path;
    buildLegacyData(legacy);
    const auto documents = mergeDocuments(legacyDocuments(legacy), imports);
    const auto version = buildDataVersion(config, documents, imports, legacy.dataFile("dict.dat"));
    publishDataVersion(config, version);
    removeUnusedVersions(config, version);
    std::cout << "Published index: " << version.dataDir << std::endl;
}

RuntimeConfig ensurePreparedData(const RuntimeConfig &config)
{
    if(std::filesystem::exists(config.dataFile("CURRENT")))
    {
        const auto current = currentVersion(config);
        if(ready(current))
        {
            return current;
        }

        const auto version = buildDataVersion(
            config, readDocuments(current.dataFile("documents.jsonl")),
            readDocuments(current.dataFile("imports.jsonl")), current.dataFile("seed.dict"));
        publishDataVersion(config, version);
        return version;
    }

    // 已有旧格式数据时直接迁移，不要求原始语料仍然存在。
    StagingDirectory stage(config.dataDir);
    auto legacy = config;
    if(!ready(config))
    {
        legacy.dataDir = stage.path;
        buildLegacyData(legacy);
    }

    const auto documents = legacyDocuments(legacy);
    const auto version = buildDataVersion(config, documents, {}, legacy.dataFile("dict.dat"));
    publishDataVersion(config, version);
    return version;
}
