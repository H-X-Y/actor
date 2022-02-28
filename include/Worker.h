#pragma once
#include <thread>
#include<Service.h>

class Server;
using namespace std;

class Worker{
    public:
        int id;     //线程编号
        int eachNum;    //每次处理消息数
        void operator()();//线程函数
        void CheckAndPutGlobal(shared_ptr<Service> srv);
};