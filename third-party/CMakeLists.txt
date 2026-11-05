cmake_minimum_required(VERSION 3.24)

project(SearchEngineThirdParty NONE)

option(SEARCH_ALLOW_DOWNLOAD "Allow missing pinned source archives to be downloaded" ON)
set(SEARCH_DEPENDENCY_CACHE "${CMAKE_BINARY_DIR}/downloads" CACHE PATH
    "Verified source archive cache"
)

if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/install" CACHE PATH
        "Third-party destination" FORCE
    )
endif()

set(SEARCH_THIRDPARTY_ROOT "${CMAKE_INSTALL_PREFIX}")
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/PrepareDependencies.cmake")
