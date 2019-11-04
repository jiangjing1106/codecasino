#include "CalculatePath.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

using namespace std;

CalculatePath::CalculatePath(int sockfd): m_socketfd(sockfd) {
    startThread();
}

CalculatePath::~CalculatePath(){
    stopThread();
}

int CalculatePath::startThread() {
    m_thrd = new thread(CalculatePath::threadMain, this);
    if (m_thrd == NULL) {
        return -1;
    }
    return 0;
}

void CalculatePath::stopThread() {
    if (m_thrd) {
        m_thrd->join();
        delete m_thrd;
        m_thrd = NULL;
    }
}

void CalculatePath::threadMain(CalculatePath* self) {
    self->runLoop();
}

void CalculatePath::runLoop() {
    while (true) {
        char* msg = fetchNextMessage();
        handleMessage(msg);
    }
}

void CalculatePath::dispatch(char* msg, int size) {
    char* recv_msg = (char*)malloc(size);
    memcpy(recv_msg, msg, size);
    unique_lock<mutex> lck(m_queue_mtx);
    m_queue.push(recv_msg);
    m_queue_cv.notify_one();
}

char* CalculatePath::fetchNextMessage() {
    unique_lock<mutex> lck(m_queue_mtx);
    while (m_queue.empty()) {
        m_queue_cv.wait(lck);
    }
    char* msg = m_queue.front();
    m_queue.pop();
    return msg;
}

void CalculatePath::handleMessage(char* msg) {
    //TODO: calculate path
#if 1   /* only for test */
    char result[3];
    sprintf(result, "%s", "[d]");
    sendMsg(result);
#endif
    free(msg);
}

int CalculatePath::sendMsg(char* msg) {
    if(send(m_socketfd, msg, strlen(msg), 0) < 0){
        printf("send msg error: %s(errno: %d)\n", strerror(errno), errno);
        return -1;
    }
    printf("send msg %s\n", msg);
    return 0;
}

