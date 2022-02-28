#include "Service.h"
#include "Server.h"
#include <iostream>
#include<unistd.h>
#include<string.h>
#include"Msg.h"
#include"ConnWriter.h"
#include"LuaAPI.h"

Service::Service(){
    pthread_spin_init(&queueLock,PTHREAD_PROCESS_PRIVATE);
    pthread_spin_init(&inGlobalLock,PTHREAD_PROCESS_PRIVATE);
}

Service::~Service(){
    pthread_spin_destroy(&queueLock);
    pthread_spin_destroy(&inGlobalLock);
}

void Service::PushMsg(shared_ptr<BaseMsg> msg){
    pthread_spin_lock(&queueLock);
    {
        msgQueue.push(msg);
    }
    pthread_spin_unlock(&queueLock);
}

shared_ptr<BaseMsg> Service::PopMsg(){
    shared_ptr<BaseMsg> msg = NULL;
    pthread_spin_lock(&queueLock);{
        if(!msgQueue.empty()){
            msg = msgQueue.front();
            msgQueue.pop();
        }
    }
    pthread_spin_unlock(&queueLock);
    return msg;
}

void Service::OnInit(){
    cout<<"["<<id<<"]OnInit"<<endl;
    luaState = luaL_newstate();
    luaL_openlibs(luaState);
    LuaAPI::Register(luaState);
    string filename = "../service/"+*type+"/init.lua";
    int isok = luaL_dofile(luaState,filename.data());
    if(isok == 1){
        cout<<"run lua fail"<<lua_tostring(luaState,-1)<<endl;
    }
    lua_getglobal(luaState,"OnInit");
    lua_pushinteger(luaState,id);
    isok = lua_pcall(luaState,1,0,0);
    if(isok != 0){
        cout<<"call lua OnInit fail"<<lua_tostring(luaState,-1)<<endl;
    }
}

void Service::OnMsg(shared_ptr<BaseMsg> msg){
    cout<<"OnMSG "<<endl;
    if(msg->type == BaseMsg::TYPE::SERVICE){
        auto m = dynamic_pointer_cast<ServiceMsg>(msg);
        OnServiceMsg(m);
    }
    else if(msg->type == BaseMsg::TYPE::SOCKET_ACCEPT){
        auto m = dynamic_pointer_cast<SocketAcceptMsg>(msg);
        OnAcceptMsg(m);
    }
    else if(msg->type == BaseMsg::TYPE::SOCKET_RW){
        auto m = dynamic_pointer_cast<SocketRWMsg>(msg);
        OnRWMsg(m);
    }
}

void Service::OnExit(){
    cout<<"["<<id<<"]OnExit"<<endl;
    lua_getglobal(luaState,"OnExit");
    int isok = lua_pcall(luaState,0,0,0);
    if(isok!=0){
        cout<<"call lua OnExit fail "<<lua_tostring(luaState,-1)<<endl;
    }
    lua_close(luaState);
}

bool Service::ProcessMsg(){
    cout<<"OnProcessMSG "<<endl;
    shared_ptr<BaseMsg> msg = PopMsg();
    if(msg){
        OnMsg(msg);
        return true;
    }else{
        return false;
    }
}

void Service::ProcessMsgs(int max){
    for(int i=0;i<max;i++){
        bool succ = ProcessMsg();
        if(!succ){
            break;
        }
    }
}

void Service::SetInGlobal(bool isIn){
    pthread_spin_lock(&inGlobalLock);
    {
        inGlobal = isIn;
    }
    pthread_spin_unlock(&inGlobalLock);
}

void Service::OnServiceMsg(shared_ptr<ServiceMsg> msg){
    cout<<"OnServiceMsg"<<endl;
    lua_getglobal(luaState,"OnServiceMsg");
    lua_pushinteger(luaState,msg->source);
    lua_pushlstring(luaState,msg->buff.get(),msg->size);
    int isok = lua_pcall(luaState,2,0,0);
    if(isok != 0){
        cout<<"call lua OnServiceMsg fail "<<lua_tostring(luaState,-1)<<endl;
    }
}

void Service::OnAcceptMsg(shared_ptr<SocketAcceptMsg> msg) {
    cout << "OnAcceptMsg " << msg->clientFd << endl;
    auto w = make_shared<ConnWriter>();
    w->fd = msg->clientFd;
    writers.emplace(msg->clientFd, w);
    lua_getglobal(luaState, "OnAcceptMsg"); 
    lua_pushinteger(luaState, msg->listenFd); 
    lua_pushinteger(luaState, msg->clientFd); 
    int isok = lua_pcall(luaState, 2, 0, 0);
    if(isok != 0){ 
         cout << "call lua OnAcceptMsg fail " << lua_tostring(luaState, -1) << endl;
    }
}

void Service::OnRWMsg(shared_ptr<SocketRWMsg> msg){
    cout<<"OnRWMSG "<<endl;
    int fd = msg->fd;
    if(msg->isRead){
        const int BUFFSIZE = 512;
        char buff[BUFFSIZE];
        int len = 0;
        do{
            len = read(fd,&buff,BUFFSIZE);
            if(len>0){
                OnSocketData(fd,buff,len);
            }
        }while(len == BUFFSIZE);
        if(len <= 0 && errno != EAGAIN){
            if (Server::inst->GetConn(fd))
            {
                OnSocketClose(fd);
                Server::inst->CloseConn(fd);
            }
        }
    }

    if(msg->isWrite){
        if(Server::inst->GetConn(fd)){
            OnSocketWriteable(fd);
        }
    }
}

void Service::OnSocketData(int fd, const char* buff, int len) {
    lua_getglobal(luaState, "OnSocketData"); 
    lua_pushinteger(luaState, fd); 
    lua_pushlstring(luaState, buff,len); 
    int isok = lua_pcall(luaState, 2, 0, 0);
    if(isok != 0){ //成功返回值为0，否则代表失败.
         cout << "call lua OnSocketData fail " << lua_tostring(luaState, -1) << endl;
    }
}

void Service::OnSocketWriteable(int fd){
    cout<<"OnSocketWriteable "<<fd<<endl;
    auto w = writers[fd];
    w->OnWriteable();
}

void Service::OnSocketClose(int fd) {
    writers.erase(fd);
    cout << "OnSocketClose " << fd << endl;
    lua_getglobal(luaState, "OnSocketClose"); 
    lua_pushinteger(luaState, fd); 
    int isok = lua_pcall(luaState, 1, 0, 0);
    if(isok != 0){ 
         cout << "call lua OnSocketClose fail " << lua_tostring(luaState, -1) << endl;
    }
}


