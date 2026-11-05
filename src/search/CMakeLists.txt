add_library(search_query
    echoServer.cpp
    dataCenter.cpp
    indexManager.cpp
    myTask.cpp
)

target_link_libraries(search_query
    PUBLIC
        search_offline
        search_text
        search_net
        search_concurrency
    PRIVATE
        SearchThirdParty::json
)
