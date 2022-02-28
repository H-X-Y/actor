#pragma once
#include<memory>
using namespace std;

class BaseMsg{
    public:
        enum TYPE{
            SERVICE = 1,
            SOCKET_ACCEPT = 2,
            SOCKET_RW = 3,
        };
        __UINT8_TYPE__ type;
        char load[999999]{}; //调试用
        virtual ~BaseMsg(){};
};

class ServiceMsg:public BaseMsg{
    public:
        __UINT32_TYPE__ source;
        shared_ptr<char> buff;
        __SIZE_TYPE__ size;
};

class SocketAcceptMsg:public BaseMsg{
    public:
        int listenFd;
        int clientFd;
};

class SocketRWMsg:public BaseMsg{
    public:
        int fd;
        bool isRead = false;
        bool isWrite = false;
};