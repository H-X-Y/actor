#include<iostream>
#include<unistd.h>
#include"Worker.h"
#include"Server.h"
using namespace std;

void Worker::operator()(){
    cout<<"Worker()"<<endl;
    while(true){
        shared_ptr<Service> srv = Server::inst->PopGlobalQueue();
        if(!srv){
            Server::inst->WorkerWait();
        }
        else{
            srv->ProcessMsgs(eachNum);
            CheckAndPutGlobal(srv);
        }
    }
}

void Worker::CheckAndPutGlobal(shared_ptr<Service> srv){
    if(srv->isExiting){
        return;
    }
    pthread_spin_lock(&srv->queueLock);
    {
        if(!srv->msgQueue.empty()){
            Server::inst->PushGlobalQueue(srv);
        }
        else{
            srv->SetInGlobal(false);
        }
    }
    pthread_spin_unlock(&srv->queueLock);
}