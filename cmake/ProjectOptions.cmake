include_guard(GLOBAL)

# 可选构建内容
option(SEARCH_BUILD_TOOLS "Build the optional prepare_data command" ON)
option(SEARCH_ENABLE_ASAN "Enable AddressSanitizer and UndefinedBehaviorSanitizer" OFF)

# 第三方依赖的位置与下载策略
option(SEARCH_ALLOW_DOWNLOAD "Allow downloading missing pinned dependency archives" ON)

set(SEARCH_THIRDPARTY_ROOT "" CACHE PATH
    "Prepared third-party installation (strict; no fallback)"
)
set(SEARCH_DEPENDENCY_CACHE "${CMAKE_BINARY_DIR}/third-party/downloads" CACHE PATH
    "Verified source archive cache"
)

# 构建类型与产物目录
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Build type" FORCE)
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/bin")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/lib")
