include_guard(GLOBAL)

if(SEARCH_ENABLE_ASAN)
    if(NOT CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        message(FATAL_ERROR "SEARCH_ENABLE_ASAN requires GCC or Clang")
    endif()

    target_compile_options(search_options INTERFACE
        -fsanitize=address,undefined
        -fno-omit-frame-pointer
    )
    target_link_options(search_options INTERFACE
        -fsanitize=address,undefined
    )
endif()
