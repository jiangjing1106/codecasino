#ifndef _CALCULATEPATH_H
#define _CALCULATEPATH_H

#include <string>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>

class CalculatePath {
public:
    CalculatePath(int sockfd);
    ~CalculatePath();

    void dispatch(char* msg, int size);

private:
    int startThread();
    void stopThread();
    static void threadMain(CalculatePath* self);
    void runLoop();
    char* fetchNextMessage();
    void handleMessage(char* msg);
    int sendMsg(char* msg);

    int parseMapDate(char* msg);

private:
    int                      m_socketfd;
    std::thread *            m_thrd;
    std::queue<char*>        m_queue;
    std::mutex               m_queue_mtx;
    std::condition_variable  m_queue_cv;

    int  m_ghost[3];     // ghost position
    int  m_player_p;     // player position
    int  m_player_d;     // player direction
    int  m_wall[30];     // wall position
};

#endif
