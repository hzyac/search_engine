add_library(search_concurrency
    taskQueue.cpp
    threadPool.cpp
    cache.cpp
)

target_link_libraries(search_concurrency
    PUBLIC
        search_common
        Threads::Threads
)
