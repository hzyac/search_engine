include_guard(GLOBAL)

if(NOT CMAKE_SYSTEM_NAME STREQUAL "Linux")
    message(FATAL_ERROR "SearchEngine currently requires Linux (epoll/eventfd).")
endif()

# 各模块通过 search_options 共享编译设置。
add_library(search_options INTERFACE)

target_compile_features(search_options INTERFACE cxx_std_17)
set(CMAKE_CXX_EXTENSIONS OFF)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(search_options INTERFACE
        -Wall
        -Wextra
        -Wpedantic
    )
endif()

find_package(Threads REQUIRED)
