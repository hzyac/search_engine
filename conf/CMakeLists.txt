# 构建目录中的配置：直接使用源码中的样例和已准备好的依赖。
configure_file(
    search.conf.in
    "${PROJECT_BINARY_DIR}/conf/search.conf"
    @ONLY
)

# 安装目录中的配置：使用相对于安装前缀的资源路径。
install(
    FILES search.conf
    DESTINATION "${CMAKE_INSTALL_SYSCONFDIR}/search_engine"
)
