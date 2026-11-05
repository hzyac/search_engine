include_guard(GLOBAL)

# 第三方运行资源及许可说明
install(
    DIRECTORY "${SEARCH_JIEBA_RESOURCE_DIR}/"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/search_engine/jieba"
)
install(
    DIRECTORY "${SEARCH_THIRDPARTY_ROOT}/share/licenses/"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/search_engine/licenses"
)
install(
    FILES "${PROJECT_SOURCE_DIR}/THIRD_PARTY.md"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/search_engine"
)

# 项目维护的资源和演示语料
install(
    DIRECTORY "${PROJECT_SOURCE_DIR}/resources/"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/search_engine/resources"
)
install(
    DIRECTORY "${PROJECT_SOURCE_DIR}/examples/corpus"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/search_engine/examples"
)
