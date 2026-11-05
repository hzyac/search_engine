include_guard(GLOBAL)
include(Dependencies)

# 选择依赖目录：显式参数 > 环境变量 > 构建目录中的自动安装。
if(NOT SEARCH_THIRDPARTY_ROOT AND DEFINED ENV{SEARCH_THIRDPARTY_ROOT})
    set(SEARCH_THIRDPARTY_ROOT "$ENV{SEARCH_THIRDPARTY_ROOT}")
endif()

if(SEARCH_THIRDPARTY_ROOT)
    get_filename_component(SEARCH_THIRDPARTY_ROOT
        "${SEARCH_THIRDPARTY_ROOT}" ABSOLUTE
    )
else()
    set(SEARCH_THIRDPARTY_ROOT "${PROJECT_BINARY_DIR}/third-party/install")

    execute_process(
        COMMAND "${CMAKE_COMMAND}"
            "-DSEARCH_THIRDPARTY_ROOT=${SEARCH_THIRDPARTY_ROOT}"
            "-DSEARCH_DEPENDENCY_CACHE=${SEARCH_DEPENDENCY_CACHE}"
            "-DSEARCH_ALLOW_DOWNLOAD=${SEARCH_ALLOW_DOWNLOAD}"
            -P "${PROJECT_SOURCE_DIR}/third-party/cmake/PrepareDependencies.cmake"
        RESULT_VARIABLE result
    )

    if(NOT result EQUAL 0)
        message(FATAL_ERROR
            "Dependency preparation failed. Check the log above, or set SEARCH_THIRDPARTY_ROOT."
        )
    endif()
endif()

# 校验安装布局和各依赖的固定版本。
set(manifest "${SEARCH_THIRDPARTY_ROOT}/versions.cmake")
if(NOT EXISTS "${manifest}")
    message(FATAL_ERROR
        "Invalid SEARCH_THIRDPARTY_ROOT: missing ${manifest}. "
        "Prepare it with cmake -S third-party -B build/deps -DCMAKE_INSTALL_PREFIX=<prefix>."
    )
endif()
include("${manifest}")

if(NOT SEARCH_INSTALLED_REVISION STREQUAL SEARCH_DEPENDENCY_REVISION)
    message(FATAL_ERROR
        "Third-party installation layout version is incompatible; prepare dependencies again."
    )
endif()

foreach(dep IN LISTS SEARCH_DEPENDENCIES)
    if(NOT "${SEARCH_INSTALLED_${dep}}" STREQUAL "${SEARCH_${dep}_COMMIT}")
        message(FATAL_ERROR
            "Third-party ${dep} version mismatch; expected ${SEARCH_${dep}_COMMIT}."
        )
    endif()
endforeach()

# 检查编译所需头文件和运行所需模型是否齐全。
foreach(required IN LISTS SEARCH_DEPENDENCY_REQUIRED_FILES)
    if(NOT EXISTS "${SEARCH_THIRDPARTY_ROOT}/${required}")
        message(FATAL_ERROR "Incomplete SEARCH_THIRDPARTY_ROOT: missing ${required}")
    endif()
endforeach()

# 业务模块通过这些目标使用依赖，无须自行维护依赖路径。
foreach(dep IN LISTS SEARCH_DEPENDENCIES)
    add_library(search_${dep} INTERFACE)
    add_library(SearchThirdParty::${dep} ALIAS search_${dep})

    target_include_directories(search_${dep} SYSTEM INTERFACE
        "${SEARCH_THIRDPARTY_ROOT}/include"
    )
endforeach()

target_link_libraries(search_simhash INTERFACE SearchThirdParty::cppjieba)
set(SEARCH_JIEBA_RESOURCE_DIR "${SEARCH_THIRDPARTY_ROOT}/share/jieba")
