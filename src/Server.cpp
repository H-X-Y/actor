#include<iostream>
#include "Server.h"
#include<unistd.h>
#include<fcntl.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<signal.h>
using namespace std;

Server* Server::inst;
Server::Server(){
    inst = this;
}

void Server::Start(){
    cout<<"Hello"<<endl;
    signal(SIGPIPE,SIG_IGN);
    pthread_rwlock_init(&servicesLock,NULL);
    pthread_spin_init(&globalLock,PTHREAD_PROCESS_PRIVATE);
    pthread_cond_init(&sleepCond,NULL);
    pthread_mutex_init(&sleepMtx,NULL);
    StartWorker();
    StartSocket();
    pthread_rwlock_init(&connsLock,NULL);
}

void Server::StartWorker(){
    for (int i = 0; i < WORKER_NUM; i++)
    {
        cout<<"start worker thread"<<i<<endl;
        Worker* worker = new Worker();
        worker->id = i;
        worker->eachNum = 2<<i;
        thread* wt = new thread(*worker);
        workers.push_back(worker);
        workerThreads.push_back(wt);
    }
    
}

void Server::Wait(){
    if(workerThreads[0]){
        workerThreads[0]->join();
    }
}

uint32_t Server::NewService(shared_ptr<string> type){
    auto srv = make_shared<Service>();
    srv->type = type;
    pthread_rwlock_wrlock(&servicesLock);
    {
        srv->id = maxId;
        maxId++;
        services.emplace(srv->id,srv);
    }
    pthread_rwlock_unlock(&servicesLock);
    srv->OnInit();
    return srv->id;
}

shared_ptr<Service> Server::GetService(uint32_t id){
    shared_ptr<Service> srv = NULL;
    pthread_rwlock_rdlock(&servicesLock);
    {
        unordered_map<uint32_t,shared_ptr<Service>>::iterator iter = services.find(id);
        if(iter != services.end()){
            srv = iter->second;
        }
    }
    pthread_rwlock_unlock(&servicesLock);
    return srv;
}

void Server::KillService(uint32_t id){
    shared_ptr<Service> srv = GetService(id);
    if(!srv){
        return;
    }
    srv->OnExit();
    srv->isExiting = true;
    pthread_rwlock_wrlock(&servicesLock);
    {
        services.erase(id);
    }
    pthread_rwlock_unlock(&servicesLock);
}

shared_ptr<Service> Server::PopGlobalQueue(){
    shared_ptr<Service> srv = NULL;
    pthread_spin_lock(&globalLock);
    {
        if(!globalQueue.empty()){
            srv = globalQueue.front();
            globalQueue.pop();
            globalLen--;
        }
    }
    pthread_spin_unlock(&globalLock);
    return srv;
}

void Server::PushGlobalQueue(shared_ptr<Service> srv){
    pthread_spin_lock(&globalLock);
    {
        globalQueue.push(srv);
        globalLen++;
    }
    pthread_spin_unlock(&globalLock);
}

void Server::Send(uint32_t toId,shared_ptr<BaseMsg> msg){
    shared_ptr<Service> toSrv = GetService(toId);
    if(!toSrv){
        cout<<"Send fail,toSrv not exist toId:"<<toId<<endl;
        return;
    }
    toSrv->PushMsg(msg);
    bool hasPush = false;
    pthread_spin_lock(&toSrv->inGlobalLock);
    {
        if(!toSrv->inGlobal){
            PushGlobalQueue(toSrv);
            toSrv->inGlobal = true;
            hasPush = true;
        }
    }
    pthread_spin_unlock(&toSrv->inGlobalLock);
    if(hasPush){
        CheckAndWeakUp();
    }
}

shared_ptr<BaseMsg> Server::MakeMsg(uint32_t source,char* buff,int len){
    auto msg = make_shared<ServiceMsg>();
    msg->type = BaseMsg::TYPE::SERVICE;
    msg->source = source;
    msg->buff = shared_ptr<char>(buff);
    msg->size = len;
    return msg;
}

void Server::WorkerWait(){
    pthread_mutex_lock(&sleepMtx);
    sleepCount++;
    pthread_cond_wait(&sleepCond,&sleepMtx);
    sleepCount--;
    pthread_mutex_unlock(&sleepMtx);
}

void Server::CheckAndWeakUp(){
    if(sleepCount == 0){
        return;
    }
    if(WORKER_NUM - sleepCount <= globalLen){
        cout<<"weakup"<<endl;
        pthread_cond_signal(&sleepCond);
    }
}

void Server::StartSocket(){
    socketWorker = new SocketWorker();
    socketWorker->Init();
    socketThread = new thread(*socketWorker);
}

int Server::AddConn(int fd,uint32_t id,Conn::TYPE type){
    auto conn = make_shared<Conn>();
    conn->fd = fd;
    conn->serviceId = id;
    conn->type = type;
    pthread_rwlock_wrlock(&connsLock);
    {
        conns.emplace(fd,conn);
    }
    pthread_rwlock_unlock(&connsLock);
    return fd;
}

shared_ptr<Conn> Server::GetConn(int fd){
    shared_ptr<Conn> conn = NULL;
    pthread_rwlock_rdlock(&connsLock);
    {
        unordered_map<uint32_t,shared_ptr<Conn>>::iterator iter = conns.find(fd);
        if(iter != conns.end()){
            conn = iter->second;
        }
    }
    pthread_rwlock_unlock(&connsLock);
    return conn;
}

bool Server::RemoveConn(int fd){
    int result;
    pthread_rwlock_wrlock(&connsLock);
    {
        result = conns.erase(fd);
    }
    pthread_rwlock_unlock(&connsLock);
    return result == 1;
}

int Server::Listen(uint32_t port,uint32_t serviceId){
    int listenFd = socket(AF_INET,SOCK_STREAM,0);
    if(listenFd <= 0){
        cout<<"listen error,listenFd <= 0"<<endl;
        return -1;
    }
    fcntl(listenFd,F_SETFL,O_NONBLOCK);
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    int r = bind(listenFd,(struct sockaddr*)&addr,sizeof(addr));
    if(r == -1){
        cout<<"listen error,bind fail"<<endl;
        return -1;
    }
    r = listen(listenFd,64);
    if(r<0){
        return -1;
    }
    AddConn(listenFd,serviceId,Conn::TYPE::LISTEN);
    socketWorker->AddEvent(listenFd);
    return listenFd;
}

void Server::CloseConn(uint32_t fd){
    bool succ = RemoveConn(fd);
    close(fd);
    if(succ){
        socketWorker->RemoveEvent(fd);
    }
}

void Server::ModifyEvent(int fd, bool epollOut) {
    socketWorker->ModifyEvent(fd, epollOut);
}