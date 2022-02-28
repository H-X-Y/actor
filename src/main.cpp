#include "Server.h"
#include<unistd.h>

int test(){
    auto pingType = make_shared<string>("ping");
    uint32_t ping1 = Server::inst->NewService(pingType);
    uint32_t ping2 = Server::inst->NewService(pingType);
    uint32_t pong = Server::inst->NewService(pingType);

    auto msg1 = Server::inst->MakeMsg(ping1,new char[3]{'h','i','\0'},3);
    auto msg2 = Server::inst->MakeMsg(ping1,new char[6]{'h','e','l','l','o','\0'},6);

    Server::inst->Send(pong,msg1);
    Server::inst->Send(pong,msg2);
}

int TestSocketCtrl(){
    int fd = Server::inst->Listen(8001,1);
    usleep(15*100000);
    Server::inst->CloseConn(fd);
}

int TestEcho(){
    auto t = make_shared<string>("gateway");
    uint32_t gateway = Server::inst->NewService(t);
}

int main(){
    //daemon(0,0);
    new Server();
    Server::inst->Start();
    auto t = make_shared<string>("main");
    Server::inst->NewService(t);
    Server::inst->Wait();
    return 0;
}