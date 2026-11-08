#pragma once

#include <cstdlib>
#include <string>

#ifndef SEARCH_ENGINE_DEFAULT_DICT_DIR
#define SEARCH_ENGINE_DEFAULT_DICT_DIR "/usr/local/share/search-engine/dict"
#endif

namespace search_engine {
inline std::string dictPath(const char* filename)
{
    const char* directory = std::getenv("SEARCH_ENGINE_DICT_DIR");
    return std::string(directory && *directory ? directory : SEARCH_ENGINE_DEFAULT_DICT_DIR)
        + "/" + filename;
}
}
