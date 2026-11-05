add_library(search_text
    splitTool.cpp
    splitToolCppJieba.cpp
)

target_link_libraries(search_text
    PUBLIC
        search_common
        SearchThirdParty::cppjieba
)
