add_library(search_net
    inetAddress.cpp
    socket.cpp
    acceptor.cpp
    socketIO.cpp
    tcpConnection.cpp
    eventLoop.cpp
    tcpServer.cpp
)

target_link_libraries(search_net
    PUBLIC
        search_common
        Threads::Threads
)
