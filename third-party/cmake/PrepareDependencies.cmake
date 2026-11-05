cmake_minimum_required(VERSION 3.24)

include("${CMAKE_CURRENT_LIST_DIR}/../../cmake/Dependencies.cmake")

if(NOT SEARCH_THIRDPARTY_ROOT OR NOT SEARCH_DEPENDENCY_CACHE)
    message(FATAL_ERROR "SEARCH_THIRDPARTY_ROOT and SEARCH_DEPENDENCY_CACHE are required")
endif()

file(MAKE_DIRECTORY "${SEARCH_THIRDPARTY_ROOT}" "${SEARCH_DEPENDENCY_CACHE}")
file(LOCK "${SEARCH_THIRDPARTY_ROOT}/.prepare.lock" GUARD PROCESS TIMEOUT 120)

# 已有安装完整且版本一致时，直接复用。
set(manifest "${SEARCH_THIRDPARTY_ROOT}/versions.cmake")
set(ready TRUE)

if(EXISTS "${manifest}")
    include("${manifest}")

    if(NOT SEARCH_INSTALLED_REVISION STREQUAL SEARCH_DEPENDENCY_REVISION)
        set(ready FALSE)
    endif()

    foreach(dep IN LISTS SEARCH_DEPENDENCIES)
        if(NOT "${SEARCH_INSTALLED_${dep}}" STREQUAL "${SEARCH_${dep}_COMMIT}")
            set(ready FALSE)
        endif()
    endforeach()

    foreach(required IN LISTS SEARCH_DEPENDENCY_REQUIRED_FILES)
        if(NOT EXISTS "${SEARCH_THIRDPARTY_ROOT}/${required}")
            set(ready FALSE)
        endif()
    endforeach()
else()
    set(ready FALSE)
endif()

if(ready)
    message(STATUS "Reusing pinned dependencies: ${SEARCH_THIRDPARTY_ROOT}")
    return()
endif()

# 获取固定版本源码包：检查缓存，必要时下载，然后校验并解压。
foreach(dep IN LISTS SEARCH_DEPENDENCIES)
    set(archive "${SEARCH_DEPENDENCY_CACHE}/${dep}-${SEARCH_${dep}_COMMIT}.tar.gz")

    if(EXISTS "${archive}")
        file(SHA256 "${archive}" actual_hash)
        if(NOT actual_hash STREQUAL SEARCH_${dep}_SHA256)
            message(FATAL_ERROR
                "Dependency archive checksum mismatch: ${archive}. "
                "Remove the damaged archive and retry."
            )
        endif()
    else()
        if(NOT SEARCH_ALLOW_DOWNLOAD)
            message(FATAL_ERROR
                "Downloads disabled; missing ${archive}. "
                "Supply a prepared SEARCH_THIRDPARTY_ROOT or populate the archive cache."
            )
        endif()

        message(STATUS "Downloading pinned ${dep}: ${SEARCH_${dep}_COMMIT}")
        file(DOWNLOAD
            "https://codeload.github.com/${SEARCH_${dep}_REPOSITORY}/tar.gz/${SEARCH_${dep}_COMMIT}"
            "${archive}.part"
            STATUS status
            TLS_VERIFY ON
            TIMEOUT 180
            INACTIVITY_TIMEOUT 30
        )
        list(GET status 0 code)

        if(NOT code EQUAL 0)
            file(REMOVE "${archive}.part")
            message(FATAL_ERROR "Download failed for ${dep}: ${status}")
        endif()

        file(SHA256 "${archive}.part" actual_hash)
        if(NOT actual_hash STREQUAL SEARCH_${dep}_SHA256)
            file(REMOVE "${archive}.part")
            message(FATAL_ERROR "Downloaded ${dep} archive checksum mismatch")
        endif()
        file(RENAME "${archive}.part" "${archive}")
    endif()

    set(extract "${SEARCH_THIRDPARTY_ROOT}/.sources/${dep}")
    file(REMOVE_RECURSE "${extract}")
    file(MAKE_DIRECTORY "${extract}")
    file(ARCHIVE_EXTRACT INPUT "${archive}" DESTINATION "${extract}")
    file(GLOB roots LIST_DIRECTORIES TRUE "${extract}/*")
    list(GET roots 0 source_${dep})
endforeach()

# 安装头文件、词典模型和上游许可说明。
file(MAKE_DIRECTORY
    "${SEARCH_THIRDPARTY_ROOT}/include"
    "${SEARCH_THIRDPARTY_ROOT}/share/jieba"
    "${SEARCH_THIRDPARTY_ROOT}/share/licenses"
)
file(COPY
    "${source_cppjieba}/include/cppjieba"
    "${source_cppjieba}/deps/limonp"
    "${source_simhash}/include/simhash"
    "${source_json}/single_include/nlohmann"
    DESTINATION "${SEARCH_THIRDPARTY_ROOT}/include"
)
file(COPY "${source_cppjieba}/dict/"
    DESTINATION "${SEARCH_THIRDPARTY_ROOT}/share/jieba"
)

foreach(dep IN LISTS SEARCH_DEPENDENCIES)
    file(GLOB notices
        "${source_${dep}}/LICENSE*"
        "${source_${dep}}/COPYING*"
        "${source_${dep}}/README*"
    )
    if(notices)
        file(COPY ${notices}
            DESTINATION "${SEARCH_THIRDPARTY_ROOT}/share/licenses/${dep}"
        )
    endif()
endforeach()

# 全部准备完成后写入版本清单，避免将中途失败的安装视为可用。
file(WRITE "${manifest}.tmp"
    "set(SEARCH_INSTALLED_REVISION ${SEARCH_DEPENDENCY_REVISION})\n"
)
foreach(dep IN LISTS SEARCH_DEPENDENCIES)
    file(APPEND "${manifest}.tmp"
        "set(SEARCH_INSTALLED_${dep} ${SEARCH_${dep}_COMMIT})\n"
    )
endforeach()
file(RENAME "${manifest}.tmp" "${manifest}")
file(REMOVE_RECURSE "${SEARCH_THIRDPARTY_ROOT}/.sources")

message(STATUS "Prepared pinned dependencies: ${SEARCH_THIRDPARTY_ROOT}")
