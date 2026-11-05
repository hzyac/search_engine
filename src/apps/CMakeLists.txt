# 对外提供的程序入口；业务实现在同级公共模块中。
add_executable(server server/main.cpp)
target_link_libraries(server PRIVATE search_query)

add_executable(client client/main.cpp)
target_link_libraries(client PRIVATE search_common SearchThirdParty::json)

install(TARGETS server client
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
)

if(SEARCH_BUILD_TOOLS)
    add_executable(prepare_data prepare_data/main.cpp)
    target_link_libraries(prepare_data PRIVATE search_offline)

    install(TARGETS prepare_data
        RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
    )

    add_custom_target(prepare-data
        COMMAND $<TARGET_FILE:prepare_data>
            --config "${PROJECT_BINARY_DIR}/conf/search.conf"
        DEPENDS prepare_data
        USES_TERMINAL
        COMMENT "Preparing all offline search data"
    )
endif()
