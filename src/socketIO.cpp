// SocketIO.cpp
#include "../include/socketIO.hpp"
#include <unistd.h>
#include <sys/socket.h>
#include <cstring>
#include <cerrno>

SocketIO::SocketIO(int fd)
: _fd(fd)
{

}

SocketIO::~SocketIO()
{

}

// 读取指定长度的数据，返回实际读取的字节数
int SocketIO::readn(char *buf, int len)
{
    int left = len;
    char *pstr = buf;

    while(left > 0)
    {
        int ret = ::read(_fd, pstr, left);
        if(ret == -1 && errno == EINTR)
        {
            continue;
        }
        else if(ret == -1)
        {
            return -1;
        }
        else if(ret == 0)
        {
            break;
        }
        else
        {
            pstr += ret;
            left -= ret;
        }
    }
    return len - left;
}

// 读取一行数据，返回实际读取的字节数
int SocketIO::readLine(char *buf, int len)
{
    int left = len - 1;
    char *pstr = buf;
    int total = 0;

    while(left > 0)
    {
        int ret = ::read(_fd, pstr, left);
        if(ret == -1 && errno == EINTR)
        {
            continue;
        }
        else if(ret == -1)
        {
            return -1;
        }
        else if(ret == 0)
        {
            break;
        }
        else
        {
            for(int idx = 0; idx < ret; ++idx)
            {
                if(pstr[idx] == '\n')
                {
                    int size = idx + 1;
                    memcpy(pstr, pstr, size);
                    pstr += size;
                    *pstr = '\0';
                    total += size;
                    return total;
                }
            }
            pstr += ret;
            left -= ret;
            total += ret;
        }
    }
    *pstr = '\0';
    return total;
}

// 写入指定长度的数据，返回实际写入的字节数
int SocketIO::writen(const char *buf, int len)
{
    int left = len;
    const char *pstr = buf;
    int ret = 0;

    while(left > 0)
    {
        ret = ::write(_fd, pstr, left);
        if(-1 == ret && errno == EINTR)
        {
            continue;
        }
        else if(-1 == ret)
        {
            return -1;
        }
        else
        {
            pstr += ret;
            left -= ret;
        }
    }
    return len - left;
}

// 小火车协议读取：前4字节为数据长度，后面跟随数据，返回实际读取的字节数
int SocketIO::readTrain(char *buf, int len)
{
    int msgLen = 0;
    int ret = readn((char *)&msgLen, 4);
    if(ret != 4)
    {
        return -1;
    }
    // 防止缓冲区溢出
    int readLen = msgLen > len ? len : msgLen;
    ret = readn(buf, readLen);
    if(ret != readLen)
    {
        return -1;
    }
    return msgLen;
}