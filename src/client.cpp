// client.cpp
#include <unistd.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>   // struct sockaddr_in、AF_INET、htons 等
#include <arpa/inet.h>    // inet_addr 等
#include <cstring>
#include <exception>
#include <iostream>
#include <nlohmann/json.hpp>

#define MAXLINE 4096

using std::cout;
using std::cerr;
using std::endl;
using std::string;

int sendn(int peerfd, const char *buf, int len)
{
    int left = len;
    const char *pStr = buf;
    int ret;

    while(left > 0)
    {
        ret = send(peerfd, pStr, left, 0);
        if(-1 == ret && errno == EINTR)
        {
            continue;
        }
        else if(-1 == ret)
        {
            cerr << "write error -1" << endl;
            return -1;
        }
        else if(0 == ret)
        {
            break;
        }
        else
        {
            pStr += ret;
            left -= ret;
        }
    }

    return len - left;
}

int recvn(int peerfd, char *buf, int len)
{
    int left = len;
    char *pbuf = buf;
    while(left > 0)
    {
        int ret = recv(peerfd, pbuf, left, 0);
        if(ret == -1 && errno == EINTR)
        {
            continue;
        }
        if(ret == -1)
        {
            cerr << "recv -1" << endl;
            return -1;
        }
        if(ret == 0)
        {
            puts("client断开连接");
            exit(0);
        }
        pbuf += ret;
        left -= ret;
    }
    return len - left;
}

int main(int argc, char * argv[])
{
    // 屏蔽信号
    signal(SIGPIPE, SIG_IGN);

    // 设置服务器信息
    struct sockaddr_in serveraddr;
    memset(&serveraddr, 0, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons(8888);
    serveraddr.sin_addr.s_addr = inet_addr("127.0.0.1");

    // 1 创建套接字
    int peerfd = socket(AF_INET, SOCK_STREAM, 0);
    if(peerfd == -1)
    {
        cerr << "Socket -1" << endl;
        return -1;
    }

    // 2 连接client
    int ret = connect(peerfd, (const struct sockaddr *)&serveraddr, sizeof(serveraddr));
    if(ret == -1)
    {
        cerr << "connect -1" << endl;
        return -1;
    }
    cout << "成功与server建立连接" << endl;

    while(1)
    {
        int type = 2; // 1代表要发送候选词, 2代表发送网页
        cout << "请输入类型, 1代表候选词, 2网页, 3退出\n";
        std::cin >> type;
        std::cin.ignore(); // 忽略 >> 留下的换行符

        // 小火车协议, 前4个代表发送的数量, 后面是内容
        if(type == 1)
        {
            // 3 发送信息
            // 封装成json 
            nlohmann::json json_object;
            cout << "请输入候选词\n";
            string query; std::getline(std::cin, query); 
            json_object["query"] = query;
            // 转换成字符串
            string sendLine = json_object.dump();
            int len = sendLine.size() + 1; // 还有一个类型占一个字节

            string msg;
            msg.append((char *)(&len), 4);
            char typeByte = static_cast<char>(type);
            msg.append(&typeByte, 1);
            msg += sendLine;
            int ret = sendn(peerfd, msg.c_str(), msg.size());
            cout << "成功发送信息, 字节数: " << ret << endl;
            cout << "内容: " << msg << endl;

            // 4 接收信息
            char recvline[MAXLINE] = {0};
            len = 0;

            // 小火车协议, 前4个字节代表, 后面是json字符串array
            ret = recvn(peerfd, (char *) &len, 4);
            if(ret == -1)
            {
                cerr << "recvn -1" << endl;
                return -1;
            }
            ret = recvn(peerfd, recvline, len);
            if(ret == -1)
            {
                cerr << "recvn -1" << endl;
                return -1;
            }
            // 解析出集合
            try {
                nlohmann::json json_array = nlohmann::json::parse(recvline);
                // 输出候选词
                cout << "收到了" << json_array.size() << "个候选词, 如下:  " << endl;
                for(string word : json_array)
                {
                    cout << word << endl;
                }
            } catch (const std::exception &e) {
                cerr << "JSON解析失败: " << e.what() << endl;
            }
        }
        else if(type == 2)
        {
            // 3 发送信息
            // 封装成json 
            nlohmann::json json_object;
            cout << "请输入网页查询关键词\n";
            string query; std::getline(std::cin, query); 
            json_object["query"] = query;
            // 转换成字符串
            string sendLine = json_object.dump();
            int len = sendLine.size() + 1; // 还有一个类型占一个字节

            string msg;
            msg.append((char *)(&len), 4);
            char typeByte = static_cast<char>(type);
            msg.append(&typeByte, 1);
            msg += sendLine;
            int ret = sendn(peerfd, msg.c_str(), msg.size());
            cout << "成功发送信息, 字节数: " << ret << endl;
            cout << "内容: " << msg << endl;

            // 4 接收信息
            len = 0;

            // 小火车协议, 前4个字节代表, 后面是json字符串array
            ret = recvn(peerfd, (char *) &len, 4);
            if(ret == -1)
            {
                cerr << "recvn -1" << endl;
                return -1;
            }

            char *recvline = new char[len + 1];
            ret = recvn(peerfd, recvline, len);
            cout << "发送过来的字节长度是:" << len << endl;
            if(ret == -1)
            {
                cerr << "recvn -1" << endl;
                delete[] recvline; 
                return -1;
            }
            recvline[len] = '\0';
            // 解析出集合
            try
            {
                nlohmann::json json_array = nlohmann::json::parse(recvline);
                // 输出网页
                cout << "收到了" << json_array.size() << "个网页, 如下:  " << endl;
                for(string WebPage : json_array)
                {
                    cout << WebPage << endl;
                }
            }
            catch (const std::exception &e)
            {
                cerr << "JSON解析失败: " << e.what() << endl;
            }

            // 释放空间
            delete[] recvline; 
        }
        else
        {
            break;
        }
    }

    // 5 关闭文件描述符
    close(peerfd);

    return 0;
}
