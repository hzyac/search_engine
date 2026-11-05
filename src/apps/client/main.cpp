#include "runtimeConfig.hpp"
#include "document.hpp"
#include <chrono>
#include <fstream>
#include <iterator>
#include <sstream>
#include <thread>
#include <nlohmann/json.hpp>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cerrno>
#include <cstdint>
#include <csignal>
#include <iostream>
#include <stdexcept>
#include <string>

namespace
{
void transfer(int fd, void *buffer, std::size_t size, bool sending)
{
    auto *bytes = static_cast<char *>(buffer);
    while(size)
    {
        auto count = sending ? ::send(fd, bytes, size, MSG_NOSIGNAL) : ::recv(fd, bytes, size, 0);
        if(count < 0 && errno == EINTR)
        {
            continue;
        }

        if(count <= 0)
        {
            throw std::runtime_error("Connection closed or socket I/O failed");
        }

        bytes += count;
        size -= static_cast<std::size_t>(count);
    }
}

nlohmann::json request(int fd, char type, const nlohmann::json &value)
{
    std::string body(1, type);
    body += value.dump();
    if(body.size() > 65535)
    {
        throw std::runtime_error("Request exceeds 65535 bytes; split the document or batch");
    }

    auto length = static_cast<std::int32_t>(body.size());
    transfer(fd, &length, sizeof(length), true);
    transfer(fd, body.data(), body.size(), true);
    transfer(fd, &length, sizeof(length), false);
    if(length <= 0 || length > 16 * 1024 * 1024)
    {
        throw std::runtime_error("Invalid response length");
    }

    std::string response(static_cast<std::size_t>(length), '\0');
    transfer(fd, response.data(), response.size(), false);
    return nlohmann::json::parse(response);
}

void checkResponse(const nlohmann::json &value)
{
    if(value.is_object() && value.contains("error"))
    {
        throw std::runtime_error(value.at("error").get<std::string>());
    }
}

void importBatch(int fd, const nlohmann::json &documents)
{
    const auto accepted = request(fd, 3, {{"documents", documents}});
    checkResponse(accepted);
    const auto id = accepted.at("task_id");
    std::cout << "导入任务 " << id << " 已接收，正在建库……" << std::endl;
    while(true)
    {
        const auto progress = request(fd, 4, {{"task_id", id}});
        checkResponse(progress);
        const auto state = progress.at("status").get<std::string>();
        if(state == "succeeded")
        {
            std::cout << "导入完成，文档已可搜索；当前文档数：" << progress.at("document_count")
                      << std::endl;
            return;
        }

        if(state == "failed")
        {
            throw std::runtime_error(progress.value("error", "Import failed"));
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void importFile(int fd, const std::filesystem::path &file)
{
    std::ifstream input(file);
    if(!input)
    {
        throw std::runtime_error("Cannot read import file: " + file.string());
    }

    const std::string text((std::istreambuf_iterator<char>(input)), {});
    auto value = nlohmann::json::parse(text, nullptr, false);
    std::vector<Document> documents;
    if(!value.is_discarded())
    {
        if(value.is_array())
        {
            for(const auto &item : value)
            {
                documents.push_back(Document::parse(item));
            }
        }
        else
        {
            documents.push_back(Document::parse(value));
        }
    }
    else
    {
        std::istringstream lines(text);
        std::string line;
        while(std::getline(lines, line))
        {
            if(line.find_first_not_of(" \t\r\n") != std::string::npos)
            {
                documents.push_back(Document::parse(nlohmann::json::parse(line)));
            }
        }
    }

    if(documents.empty())
    {
        throw std::runtime_error("Import file contains no documents");
    }

    // 保留现有帧大小，较大文件分批发送，每批成功后再继续。
    auto batch = nlohmann::json::array();
    std::size_t bytes = 32;
    for(const auto &document : documents)
    {
        auto item = document.json();
        const auto size = item.dump().size() + 1;
        if(size + 32 > 60000)
        {
            throw std::runtime_error("Document exceeds the import limit: " + document.id);
        }

        if(bytes + size > 60000)
        {
            importBatch(fd, batch);
            batch = nlohmann::json::array();
            bytes = 32;
        }

        batch.push_back(std::move(item));
        bytes += size;
    }

    if(!batch.empty())
    {
        importBatch(fd, batch);
    }
}

struct SocketHandle
{
    int fd = -1;

    ~SocketHandle()
    {
        if(fd >= 0)
        {
            ::close(fd);
        }
    }
};
}

int main(int argc, char **argv)
{
    try
    {
        std::optional<std::filesystem::path> configFile;
        std::optional<std::string> host;
        std::optional<unsigned short> port;
        std::optional<std::filesystem::path> importPath;

        const auto usage = "Usage: client [--config <file>] [--host <IPv4>] [--port <port>] "
                           "[--import <JSON/JSONL>]";
        if(argc == 2 && std::string(argv[1]) == "--help")
        {
            std::cout << usage << '\n';
            return 0;
        }

        for(int i = 1; i < argc; i += 2)
        {
            if(i + 1 >= argc || std::string(argv[i + 1]).empty())
            {
                throw std::runtime_error(usage);
            }

            const std::string option = argv[i];
            if(option == "--config" && !configFile)
            {
                configFile = argv[i + 1];
            }
            else if(option == "--host" && !host)
            {
                host = argv[i + 1];
            }
            else if(option == "--port" && !port)
            {
                port = parsePort(argv[i + 1]);
            }
            else if(option == "--import" && !importPath)
            {
                importPath = argv[i + 1];
            }
            else
            {
                throw std::runtime_error(usage);
            }
        }

        if(!configFile)
        {
            configFile = defaultConfigFile();
        }

        auto config = configFile ? RuntimeConfig::load(*configFile) : RuntimeConfig{};
        if(host)
        {
            config.host = *host;
        }

        if(port)
        {
            config.port = *port;
        }

        config.validateEndpoint();

        SocketHandle socket{::socket(AF_INET, SOCK_STREAM, 0)};
        if(socket.fd < 0)
        {
            throw std::runtime_error("Cannot create socket");
        }

        sockaddr_in address{};
        address.sin_family = AF_INET;
        address.sin_port = htons(config.port);
        inet_pton(AF_INET, config.host.c_str(), &address.sin_addr);
        if(::connect(socket.fd, reinterpret_cast<sockaddr *>(&address), sizeof(address)) < 0)
        {
            throw std::runtime_error("Cannot connect to " + config.host + ":" +
                                     std::to_string(config.port));
        }

        std::cout << "成功与server建立连接\n";
        if(importPath)
        {
            importFile(socket.fd, *importPath);
            return 0;
        }

        std::string line;
        while(std::cout << "请选择：1 候选词，2 网页搜索，3 退出，4 导入文档，5 导入状态\n",
              std::getline(std::cin, line))
        {
            if(line == "3" || line == "quit" || line == "exit")
            {
                break;
            }

            if(line == "4")
            {
                std::cout << "请输入 JSON 或 JSONL 文件路径\n";
                if(!std::getline(std::cin, line))
                {
                    break;
                }

                try
                {
                    importFile(socket.fd, line);
                }
                catch(const std::exception &error)
                {
                    std::cerr << "导入失败：" << error.what() << '\n';
                }
                continue;
            }

            if(line == "5")
            {
                std::cout << request(socket.fd, 4, nlohmann::json::object()).dump(2, ' ', false)
                          << '\n';
                continue;
            }

            if(line != "1" && line != "2")
            {
                std::cerr << "Invalid query type\n";
                continue;
            }

            const char type = line == "1" ? 1 : 2;
            std::cout << "请输入查询词\n";
            if(!std::getline(std::cin, line))
            {
                break;
            }

            const auto result = request(socket.fd, type, {{"query", line}});
            std::cout << result.dump(2, ' ', false) << '\n';
        }

        return 0;
    }
    catch(const std::exception &error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
