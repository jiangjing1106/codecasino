#ifndef _CALCULATEPATH_H
#define _CALCULATEPATH_H

#include <string>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>

#if 1
typedef struct _position
{
	int x;
	int y;
}Pos;
#else
struct Pos
{
	int x;
	int y;

}
#endif
class CalculatePath {
public:
    CalculatePath(int sockfd);
    ~CalculatePath();

    int dispatch(char* msg, int size);

private:
    int startThread();
    void stopThread();
    static void threadMain(CalculatePath* self);
    void runLoop();
    char* fetchNextMessage();
    void handleMessage(char* msg);
    int sendMsg(char* msg);

    int parseMapDate(char* msg);
	void positionSet(Pos* buff,int ID);
	int NextMove(char* msg);
#if 0
	Pos findHighestScore(char* msg);//3*3
#endif
	char getPositionObject_Toplayer(int x, int y, char* msg);
	char getDirObjectA(char* msg);
	char getDirObjectS(char* msg);
	char getDirObjectD(char* msg);
	char getDirObjectW(char* msg);
	

	int IsAvailableObject(char object);
	

private:
    int                      m_socketfd;
    std::thread *            m_thrd;
    std::queue<char*>        m_queue;
    std::mutex               m_queue_mtx;
    std::condition_variable  m_queue_cv;

    //int  m_ghost[3];     // ghost position
    Pos m_ghost[3];
	Pos m_player_p;
	Pos m_wall[30];
	Pos m_nextTarget;
    int  _m_player_p;     // player position
    int  m_player_d;     // player direction
    //int  m_wall[30];     // wall position
};

#endif
