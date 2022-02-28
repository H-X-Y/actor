#pragma once
#include<vector>
#include<thread>
#include"Worker.h"
#include"Service.h"
#include<unordered_map>
#include"SocketWorker.h"
#include"Conn.h"

class Worker;
class Server{
    public:
        static Server* inst;

        unordered_map<uint32_t,shared_ptr<Service>> services;
        uint32_t maxId = 0;
        pthread_rwlock_t servicesLock;
    public:
        Server();

        void Start();

        void Wait();

        uint32_t NewService(shared_ptr<string> type);

        void KillService(uint32_t id);

        void Send(uint32_t toId,shared_ptr<BaseMsg> msg);

        shared_ptr<Service> PopGlobalQueue();

        void PushGlobalQueue(shared_ptr<Service> srv);

        shared_ptr<BaseMsg> MakeMsg(uint32_t source,char* buff,int len);

        void CheckAndWeakUp();//唤醒工作线程

        void WorkerWait();//让工作线程等待

        int AddConn(int fd,uint32_t id,Conn::TYPE type);

        shared_ptr<Conn> GetConn(int fd);

        bool RemoveConn(int fd);

        int Listen(uint32_t port,uint32_t serviceId);

        void CloseConn(uint32_t fd);

        void ModifyEvent(int fd, bool epollOut);
    private:
        int WORKER_NUM = 3; //工作线程数
        vector<Worker*> workers; //worker对象
        vector<thread*> workerThreads; //线程

        queue<shared_ptr<Service>> globalQueue;
        int globalLen = 0;
        pthread_spinlock_t globalLock;

        pthread_mutex_t sleepMtx;//工作线程的休眠与唤醒
        pthread_cond_t sleepCond;
        int sleepCount = 0;
        
        SocketWorker* socketWorker;
        thread* socketThread;

        unordered_map<uint32_t,shared_ptr<Conn>>conns;
        pthread_rwlock_t connsLock;
    private:
        void StartWorker();

        shared_ptr<Service> GetService(uint32_t id);

        void StartSocket();
};