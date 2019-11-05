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

enum DIRECTION {
    UP = 0,
    DOWN,
    LEFT,
    RIGHT
};

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

int CalculatePath::sendMsg(char* msg) {
    if (send(m_socketfd, msg, strlen(msg), 0) < 0){
        printf("send msg error: %s(errno: %d)\n", strerror(errno), errno);
        return -1;
    }
    printf("send msg %s\n", msg);
    return 0;
}

void CalculatePath::handleMessage(char* msg) {
    //TODO: calculate path
#if 0   /* only for test */
    char result[3];
    sprintf(result, "%s", "[d]");
    sendMsg(result);
#endif
    parseMapDate(msg);
    free(msg);
}

int CalculatePath::parseMapDate(char* msg) {
    int size = strlen(msg);
    if ((size != 227) || (msg[0] != '[') || (msg[226] != ']')) {
        printf("msg %s is invaild! size = %d\n", msg, size);
        return -1;
    }

    int wall_count = 0;
    int ghost_count = 0;
    for(int i=0; i<size; i++) {
        if (msg[i] == '9') {  // wall
            m_wall[wall_count] = i;
            printf("wall count is [%d], position is [%d]\n", wall_count, i);
            wall_count++;
        } else if (msg[i] == 'G') {  // ghost
            m_ghost[ghost_count] = i;
            printf("ghost count is [%d], position is [%d]\n", ghost_count, i);
            ghost_count++;
        } else if (msg[i] == 'w') {  // player up
            m_player_d = UP;
            m_player_p = i;
            printf("player position is [%d]\n", i);
        } else if (msg[i] == 'a') {  // player left
            m_player_d = LEFT;
            m_player_p = i;
            printf("player position is [%d]\n", i);
        } else if (msg[i] == 's') {  // player down
            m_player_d = DOWN;
            m_player_p = i;
            printf("player position is [%d]\n", i);
        } else if (msg[i] == 'd') {  // player right
            m_player_d = RIGHT;
            m_player_p = i;
            printf("player position is [%d]\n", i);
        }
    }
    return 0;
}

