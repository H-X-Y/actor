#include"LuaAPI.h"
#include"stdint.h"
#include"Server.h"
#include<unistd.h>
#include<string.h>
#include<iostream>

int LuaAPI::NewService(lua_State* luaState){
    int num = lua_gettop(luaState);
    if(lua_isstring(luaState,1)==0){
        lua_pushinteger(luaState,-1);
        return 1;
    }
    size_t len = 0;
    const char *type = lua_tolstring(luaState,1,&len);
    char* newstr = new char[len+1];
    newstr[len] = '\0';
    memcpy(newstr,type,len);
    auto t = make_shared<string>(newstr);
    uint32_t id = Server::inst->NewService(t);
    lua_pushinteger(luaState,id);
    return 1;
}

void LuaAPI::Register(lua_State *luaState){
    static luaL_Reg lualibs[]{
        {"NewService",NewService},
        {"KillService",KillService},
        {"Send",Send},
        {"Listen",Listen},
        {"CloseConn",CloseConn},
        {"Write",Write},
        {NULL,NULL}
    };
    luaL_newlib(luaState,lualibs);
    lua_setglobal(luaState,"server");
}

int LuaAPI::KillService(lua_State* luaState){
    int num = lua_gettop(luaState);
    if(lua_isinteger(luaState,1)==0){
        return 0;
    }
    int id = lua_tointeger(luaState,1);
    Server::inst->KillService(id);
    return 0;
}

int LuaAPI::Send(lua_State* luaState){
    int num = lua_gettop(luaState);
    if(num != 3){
        cout<<"Send fail,num err "<<endl;
        return 0;
    }
    if(lua_isinteger(luaState,1)==0){
        cout<<"Send fail,num err"<<endl;
        return 0;
    }
    int source = lua_tointeger(luaState,1);
    if(lua_isinteger(luaState,2)==0){
        cout<<"Send fail,arg2 err"<<endl;
        return 0;
    }
    int toId = lua_tointeger(luaState,2);
    if(lua_isstring(luaState,3)==0){
        cout<<"Send fail,arg3 err"<<endl;
        return 0;
    }
    size_t len = 0;
    const char* buff = lua_tolstring(luaState,3,&len);
    char* newstr = new char[len];
    memcpy(newstr,buff,len);
    auto msg = make_shared<ServiceMsg>();
    msg->type = BaseMsg::TYPE::SERVICE;
    msg->source = source;
    msg->buff = shared_ptr<char>(newstr);
    msg->size = len;
    Server::inst->Send(toId,msg);
    return 0;
}

int LuaAPI::Write(lua_State* luaState){
    int num = lua_gettop(luaState);
    if(lua_isinteger(luaState,1)==0){
        lua_pushinteger(luaState,-1);
        return 1;
    }
    int fd = lua_tointeger(luaState,1);
    if(lua_isstring(luaState,2)==0){
        lua_pushinteger(luaState,-1);
        return 1;
    }
    size_t len = 0;
    const char* buff = lua_tolstring(luaState,2,&len);
    int r = write(fd,buff,len);
    lua_pushinteger(luaState,r);
    return 1;
}

int LuaAPI::Listen(lua_State *luaState){
    int num = lua_gettop(luaState);
    if(lua_isinteger(luaState, 1) == 0) {
        lua_pushinteger(luaState, -1);
        return 1;
    }
    int port = lua_tointeger(luaState, 1);
    if(lua_isinteger(luaState, 2) == 0) {
        lua_pushinteger(luaState, -1);
        return 1;
    }
    int id = lua_tointeger(luaState, 2);
    int fd = Server::inst->Listen(port, id);
    lua_pushinteger(luaState, fd);
    return 1;
}

int LuaAPI::CloseConn(lua_State *luaState){
    int num = lua_gettop(luaState);
    if(lua_isinteger(luaState, 1) == 0) {
        return 0;
    }
    int fd = lua_tointeger(luaState, 1);
    Server::inst->CloseConn(fd);
    return 0;
}