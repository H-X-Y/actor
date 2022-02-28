#include"SocketWorker.h"
#include"Server.h"
#include<iostream>
#include<unistd.h>
#include<assert.h>
#include<string.h>
#include<fcntl.h>
#include<sys/socket.h>

void SocketWorker::Init(){
    cout<<"SocketWorker Init"<<endl;
    epollFd = epoll_create(1024);
    assert(epollFd > 0);
}



void SocketWorker::AddEvent(int fd){
    cout<<"AddEvent fd"<<fd<<endl;
    struct epoll_event ev;
    ev.events = EPOLLIN||EPOLLET;
    ev.data.fd = fd;
    if(epoll_ctl(epollFd,EPOLL_CTL_ADD,fd,&ev)==-1){
        cout<<"AddEvent epoll_ctl Fail:"<<strerror(errno)<<endl;
    }
}

void SocketWorker::RemoveEvent(int fd){
    cout<<"RemoveEvent fd "<<fd<<endl;
    epoll_ctl(epollFd,EPOLL_CTL_DEL,fd,NULL);
}

void SocketWorker::ModifyEvent(int fd,bool epollOut){
    cout<<"ModifyEvent fd "<<fd<<" "<<epollOut<<endl;
    struct epoll_event ev;
    ev.data.fd = fd;
    if(epollOut){
        ev.events = EPOLLIN|EPOLLET|EPOLLOUT;
    }
    else{
        ev.events = EPOLLIN|EPOLLET;
    }
    epoll_ctl(epollFd,EPOLL_CTL_MOD,fd,&ev);
}

void SocketWorker::OnEvent(epoll_event ev){
    int fd = ev.data.fd;
    auto conn = Server::inst->GetConn(fd);
    if(conn == NULL){
        cout<<"OnEvent error,conn == NULL"<<endl;
        return;
    }
    bool isRead = ev.events&EPOLLIN;
    bool isWrite = ev.events&EPOLLOUT;
    bool isError = ev.events&EPOLLERR;
    if(conn->type == Conn::TYPE::LISTEN){
        if(isRead){
            OnAccept(conn);
        }
    }
    else{
        if(isRead || isWrite){
            OnRW(conn,isRead,isWrite);
        }
        if(isError){
            cout<<"OnError fd:"<<conn->fd<<endl;
        }
    }
}

void SocketWorker::operator()(){
    while (true)
    {
        const int EVENT_SIZE = 64;
        struct epoll_event events[EVENT_SIZE];
        int eventCount = epoll_wait(epollFd,events,EVENT_SIZE,-1);
        for(int i=0;i<eventCount;i++){
            epoll_event ev = events[i];
            OnEvent(ev);
        }
    }
}

void SocketWorker::OnAccept(shared_ptr<Conn> conn){
    cout<<"OnAccept fd:"<<conn->fd<<endl;
    int clientFd = accept(conn->fd,NULL,NULL);
    if(clientFd<0){
        cout<<"accept error"<<endl;
    }
    fcntl(clientFd,F_SETFL,O_NONBLOCK);
    Server::inst->AddConn(clientFd,conn->serviceId,Conn::TYPE::CLIENT);
    struct epoll_event ev;
    ev.events = EPOLLIN|EPOLLET;
    ev.data.fd = clientFd;
    if(epoll_ctl(epollFd,EPOLL_CTL_ADD,clientFd,&ev)==-1){
        cout<<"OnAccept epoll_ctl Fail:"<<strerror(errno)<<endl;
    }
    auto msg = make_shared<SocketAcceptMsg>();
    msg->type = BaseMsg::TYPE::SOCKET_ACCEPT;
    msg->listenFd = conn->fd;
    msg->clientFd = clientFd;
    Server::inst->Send(conn->serviceId,msg);
}

void SocketWorker::OnRW(shared_ptr<Conn> conn,bool r,bool w){
    cout<<"OnRW fd:"<<conn->fd<<endl;
    auto msg = make_shared<SocketRWMsg>();
    msg->type = BaseMsg::TYPE::SOCKET_RW;
    msg->fd = conn->fd;
    msg->isRead = r;
    msg->isWrite = w;
    Server::inst->Send(conn->serviceId,msg);
}