#pragma once
#include<queue>
#include<thread>
#include<unordered_map>
#include "Msg.h"
#include"ConnWriter.h"
extern "C"{
    #include "lua.h"
    #include "lauxlib.h"
    #include "lualib.h"
}

using namespace std;

class Service{
    public:
        uint32_t id;
        shared_ptr<string> type;
        bool isExiting = false;//是否正在退出
        queue<shared_ptr<BaseMsg>>msgQueue;
        pthread_spinlock_t queueLock;

        bool inGlobal = false;//是否在全局服务队列里
        pthread_spinlock_t inGlobalLock;
        
    public:
        Service();
        ~Service();

        void OnInit();
        void OnMsg(shared_ptr<BaseMsg> msg);
        void OnExit();

        void PushMsg(shared_ptr<BaseMsg> msg);
        bool ProcessMsg();
        void ProcessMsgs(int max);

        void SetInGlobal(bool isIn);

        
    private:
        shared_ptr<BaseMsg> PopMsg();

        void OnServiceMsg(shared_ptr<ServiceMsg> msg);
        void OnAcceptMsg(shared_ptr<SocketAcceptMsg> msg);
        void OnRWMsg(shared_ptr<SocketRWMsg> msg);
        void OnSocketData(int fd,const char* buff,int len);
        void OnSocketWriteable(int fd);
        void OnSocketClose(int fd);

        unordered_map<int,shared_ptr<ConnWriter>> writers;//测试ConnWriter模块用

        lua_State *luaState;//lua虚拟机
};