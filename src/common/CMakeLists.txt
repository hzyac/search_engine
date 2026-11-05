add_library(search_common
    document.cpp
    runtimeConfig.cpp
    nonCopyable.cpp
)

target_include_directories(search_common
    PUBLIC "${PROJECT_SOURCE_DIR}/include/search_engine"
)

target_link_libraries(search_common
    PUBLIC search_options SearchThirdParty::json
)
