#include "ConnWriter.h"
#include <unistd.h>
#include <Server.h>
#include <iostream>
#include <string.h>
#include <sys/socket.h>

void ConnWriter::EntireWriteWhenEmpty(shared_ptr<char> buff, streamsize len) {
    char* s = buff.get() ;
    //>=0, -1&&EAGAIN, -1&&EINTR, -1&&其他
    streamsize n = write(fd, s, len);
    if(n < 0 && errno == EINTR) { }; 
    cout << "EntireWriteWhenEmpty write n=" << n << endl;
    //情况1-1：全部写完
    if(n >= 0 && n == len) {
        return;
    }
    //情况1-2：写一部分（或没写入）
    if( (n > 0 && n < len) || (n < 0 && errno == EAGAIN) ) {
        auto obj = make_shared<WriteObject>();
        obj->start = n;
        obj->buff = buff;
        obj->len = len;
        objs.push_back(obj);
        Server::inst->ModifyEvent(fd, true);
        return;
    }
    //情况1-3：真的发生错误
    cout << "EntireWrite write error " <<  endl;
}

void ConnWriter::EntireWriteWhenNotEmpty(shared_ptr<char> buff, streamsize len) {
    auto obj = make_shared<WriteObject>();
    obj->start = 0;
    obj->buff = buff;
    obj->len = len;
    objs.push_back(obj);
}

void ConnWriter::EntireWrite(shared_ptr<char> buff, streamsize len) {
    if(isClosing){
        cout << "EntireWrite fail, because isClosing" << endl;
        return;
    }
    if(objs.empty()) {
        EntireWriteWhenEmpty(buff, len);
    }
    else{
        EntireWriteWhenNotEmpty(buff, len);
    }
}



bool ConnWriter::WriteFrontObj() {
    if(objs.empty()) {
        return false;
    }
    auto obj = objs.front();
    char* s = obj->buff.get() + obj->start;
    int len = obj->len - obj->start;
    int n = write(fd, s, len);
    cout << "WriteFrontObj write n=" << n << endl;
    if(n < 0 && errno == EINTR) { }; 
    if(n >= 0 && n == len) {
        objs.pop_front(); 
        return true;
    }
    if( (n > 0 && n < len) || (n < 0 && errno == EAGAIN) ) {
        obj->start += n;
        return false;
    }
    cout << "EntireWrite write error " << endl;
}

void ConnWriter::OnWriteable() {
    auto conn = Server::inst->GetConn(fd);
    if(conn == NULL){ 
        return;
    }

    while(WriteFrontObj()){}
    
    if(objs.empty()) {
        Server::inst->ModifyEvent(fd, false);

        if(isClosing) {
            cout << "linger close conn" << endl;
            shutdown(fd, SHUT_RD);
            auto msg= make_shared<SocketRWMsg>();
            msg->type = BaseMsg::TYPE::SOCKET_RW;
            msg->fd = conn->fd;
            msg->isRead = true;
            Server::inst->Send(conn->serviceId, msg);
        }
    }
}

void ConnWriter::LingerClose(){
    if(isClosing){
        return;
    }
    isClosing = true;
    if(objs.empty()) {
        Server::inst->CloseConn(fd);
        return;
    }
}