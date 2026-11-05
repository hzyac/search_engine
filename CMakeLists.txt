cmake_minimum_required(VERSION 3.24)

project(SearchEngine VERSION 0.1.0 LANGUAGES CXX)

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

# 公共构建配置
include(ProjectOptions)      # 构建开关、默认配置、输出目录
include(CompilerOptions)     # 平台检查、C++ 标准、编译警告
include(Sanitizers)          # 可选的 ASan / UBSan
include(ThirdPartyConfig)    # 固定版本的第三方依赖
include(GNUInstallDirs)

# 项目模块与运行配置
add_subdirectory(src)
add_subdirectory(conf)

# 运行资源与依赖许可的安装规则
include(InstallRules)

message(STATUS "SearchEngine: ${CMAKE_BUILD_TYPE}, tools=${SEARCH_BUILD_TOOLS}")
message(STATUS "Third-party root: ${SEARCH_THIRDPARTY_ROOT}")
message(STATUS "Generated example config: ${PROJECT_BINARY_DIR}/conf/search.conf")
