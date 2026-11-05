// SocketIO.hpp
#ifndef SEARCH_ENGINE_SOCKET_IO_HPP
#define SEARCH_ENGINE_SOCKET_IO_HPP

class SocketIO
{
public:
    // 构造函数和析构函数
    explicit SocketIO(int fd);
    ~SocketIO();

    // 读取指定长度的数据，返回实际读取的字节数
    int readn(char *buf, int len);

    // 读取一行数据，返回实际读取的字节数
    int readLine(char *buf, int len);

    // 小火车协议读取：前4字节为数据长度，后面跟随数据，返回实际读取的字节数
    int readTrain(char *buf, int len);

    // 写入指定长度的数据，返回实际写入的字节数
    int writen(const char *buf, int len);

private:
    int _fd;
};

#endif
