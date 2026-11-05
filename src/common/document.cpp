#include "document.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <map>
#include <stdexcept>

namespace
{
std::string field(const nlohmann::json &value, const char *name, bool required)
{
    if(!value.contains(name) && !required)
    {
        return {};
    }

    if(!value.contains(name) || !value.at(name).is_string())
    {
        throw std::runtime_error(std::string("Document field must be a string: ") + name);
    }

    auto result = value.at(name).get<std::string>();
    if(result.find('\0') != std::string::npos ||
       (required && result.find_first_not_of(" \t\r\n") == std::string::npos))
    {
        throw std::runtime_error(std::string("Invalid document field: ") + name);
    }

    return result;
}

std::string escape(const std::string &text)
{
    std::string result;
    for(char ch : text)
    {
        switch(ch)
        {
        case '&':
            result += "&amp;";
            break;
        case '<':
            result += "&lt;";
            break;
        case '>':
            result += "&gt;";
            break;
        default:
            result += ch;
            break;
        }
    }

    return result;
}
}

Document Document::parse(const nlohmann::json &value)
{
    if(!value.is_object())
    {
        throw std::runtime_error("Each document must be a JSON object");
    }

    Document document;
    document.id = field(value, "id", true);
    document.title = field(value, "title", false);
    document.url = field(value, "url", false);
    document.content = field(value, "content", true);
    if(document.id.size() > 512 || document.id.find_first_of("\r\n") != std::string::npos)
    {
        throw std::runtime_error("Document id must be at most 512 bytes without line breaks");
    }

    return document;
}

nlohmann::json Document::json() const
{
    return {{"id", id}, {"title", title}, {"url", url}, {"content", content}};
}

std::string Document::page() const
{
    return "<doc>\n\t<id>" + escape(id) + "</id>\n\t<url>" + escape(url) + "</url>\n\t<title>" +
           escape(title) + "</title>\n\t<content>" + escape(content) + "</content>\n</doc>\n\n";
}

std::vector<Document> readDocuments(const std::filesystem::path &file)
{
    std::ifstream input(file);
    if(!input)
    {
        throw std::runtime_error("Cannot read document store: " + file.string());
    }

    std::vector<Document> result;
    std::string line;
    while(std::getline(input, line))
    {
        if(line.find_first_not_of(" \t\r\n") != std::string::npos)
        {
            result.push_back(Document::parse(nlohmann::json::parse(line)));
        }
    }

    if(input.bad())
    {
        throw std::runtime_error("Cannot finish reading document store: " + file.string());
    }

    return result;
}

void writeDocuments(const std::filesystem::path &file, const std::vector<Document> &documents)
{
    std::ofstream output(file);
    output.exceptions(std::ios::badbit | std::ios::failbit);
    for(const auto &document : documents)
    {
        output << document.json().dump() << '\n';
    }

    output.close();
}

std::vector<Document> mergeDocuments(const std::vector<Document> &current,
                                     const std::vector<Document> &updates)
{
    std::map<std::string, Document> documents;
    for(const auto &document : current)
    {
        documents[document.id] = document;
    }

    for(const auto &document : updates)
    {
        documents[document.id] = document;
    }

    std::vector<Document> result;
    for(auto &entry : documents)
    {
        result.push_back(std::move(entry.second));
    }

    return result;
}

void writeJson(const std::filesystem::path &file, const nlohmann::json &value)
{
    const auto temporary = file.string() + ".tmp";
    std::ofstream output(temporary);
    output.exceptions(std::ios::badbit | std::ios::failbit);
    output << value.dump() << '\n';
    output.close();
    std::filesystem::rename(temporary, file);
}
