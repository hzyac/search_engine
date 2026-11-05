#ifndef SEARCH_ENGINE_DOCUMENT_HPP
#define SEARCH_ENGINE_DOCUMENT_HPP

#include <nlohmann/json_fwd.hpp>
#include <filesystem>
#include <string>
#include <vector>

struct Document
{
    std::string id;
    std::string title;
    std::string url;
    std::string content;

    static Document parse(const nlohmann::json &value);
    nlohmann::json json() const;
    std::string page() const;
};

std::vector<Document> readDocuments(const std::filesystem::path &file);
void writeDocuments(const std::filesystem::path &file, const std::vector<Document> &documents);
std::vector<Document> mergeDocuments(const std::vector<Document> &current,
                                     const std::vector<Document> &updates);
void writeJson(const std::filesystem::path &file, const nlohmann::json &value);

#endif
