#include "dataCenter.hpp"
#include "document.hpp"
#include <cmath>
#include <fstream>
#include <sstream>
#include <stdexcept>

DataCenter *DataCenter::getInstance()
{
    static DataCenter instance;
    return &instance;
}

std::shared_ptr<const SearchData> DataCenter::read(const RuntimeConfig &config)
{
    config.validateServerData();

    auto data = std::make_shared<SearchData>();
    data->_config = config;
    data->_generation = config.dataDir.filename().string();
    data->_dict.push_back({"", 0});
    data->_webPageOffset.push_back({0, 0});

    std::ifstream dict(config.dataFile("dict.dat"));
    std::string line;
    while(std::getline(dict, line))
    {
        std::istringstream input(line);
        std::string word;
        int count;
        if(!(input >> word >> count) || count < 0)
        {
            throw std::runtime_error("Invalid dictionary row");
        }

        data->_dict.emplace_back(word, count);
    }

    std::ifstream index(config.dataFile("dictIndex.dat"));
    while(std::getline(index, line))
    {
        std::istringstream input(line);
        std::string word;
        int id;
        if(!(input >> word))
        {
            throw std::runtime_error("Invalid dictionary index row");
        }
        while(input >> id)
        {
            if(id <= 0 || static_cast<std::size_t>(id) >= data->_dict.size())
            {
                throw std::runtime_error("Invalid dictionary ID");
            }

            data->_index[word].insert(id);
        }
    }

    const auto pageSize = std::filesystem::file_size(config.dataFile("newripepage.dat"));
    std::ifstream offsets(config.dataFile("newoffset.dat"));
    std::size_t id, position, length;
    while(offsets >> id >> position >> length)
    {
        if(id != data->_webPageOffset.size() || position > pageSize || length == 0 ||
           length > pageSize - position)
        {
            throw std::runtime_error("Invalid webpage offset");
        }

        data->_webPageOffset.emplace_back(position, length);
    }

    std::ifstream inverted(config.dataFile("invertIndex.dat"));
    std::string word;
    double weight;
    while(inverted >> word >> id >> weight)
    {
        if(id == 0 || id >= data->_webPageOffset.size() || !std::isfinite(weight))
        {
            throw std::runtime_error("Invalid inverted index entry");
        }

        data->_invertedIndex[word].emplace(static_cast<int>(id), weight);
    }

    if(data->_dict.size() <= 1 || data->_webPageOffset.size() <= 1 || data->_invertedIndex.empty())
    {
        throw std::runtime_error("Server data is empty or malformed; rebuild offline data");
    }

    if(readDocuments(config.dataFile("documents.jsonl")).size() + 1 != data->_webPageOffset.size())
    {
        throw std::runtime_error("Document store and webpage offsets disagree");
    }

    return data;
}

SearchData::~SearchData()
{
    if(_retired.load())
    {
        std::error_code error;
        std::filesystem::remove_all(_config.dataDir, error);
    }
}

std::shared_ptr<const SearchData> DataCenter::snapshot() const
{
    return std::atomic_load(&_data);
}

void DataCenter::publish(std::shared_ptr<const SearchData> data) noexcept
{
    auto previous = std::atomic_exchange(&_data, data);
    if(previous && previous->_config.dataDir != data->_config.dataDir)
    {
        previous->_retired.store(true);
    }
}
