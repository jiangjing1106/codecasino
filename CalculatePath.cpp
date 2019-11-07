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
#include <iostream>

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

int CalculatePath::dispatch(char* msg, int size) {
    if ((size != 227) || (msg[0] != '[') || (msg[226] != ']')) {
        printf("msg %s is invaild! size = %d\n", msg, size);
        return -1;
    }
    char* recv_msg = (char*)malloc(size);
    memcpy(recv_msg, msg+1, size-2);
    unique_lock<mutex> lck(m_queue_mtx);
    m_queue.push(recv_msg);
    m_queue_cv.notify_one();
	return 0;
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
	char result[3];
	int dir = 0;
	static int count= 0;
    parseMapDate(msg);
	dir = NextMove(msg);
	switch(dir){
		case UP:
			sprintf(result, "%s", "[w]");
			break;
		case DOWN:
			sprintf(result, "%s", "[s]");
			break;
		case LEFT:
			sprintf(result, "%s", "[a]");
			break;
		case RIGHT:
			sprintf(result, "%s", "[d]");
			break;
		default:
			sprintf(result, "%s", "[d]");
			break;
	}
	cout<<"!!!!!!!!!! count:"<<count<<"   move:"<<dir<<endl;
	count ++;
	sendMsg(result);

    free(msg);
}

int CalculatePath::parseMapDate(char* msg) {
    int size = strlen(msg);

    int wall_count = 0;
    int ghost_count = 0;

	cout<<"msg size:"<<endl;
    for(int i=0; i<size; i++) {
        if (msg[i] == '9') {  // wall
        	positionSet(&m_wall[wall_count],i);
			//cout<<"wall pos["<<m_wall[wall_count].x+1<<","<<m_wall[wall_count].y+1<<"]"<<endl;
			wall_count++;
        } else if (msg[i] == 'G') {  // ghost
			positionSet(&m_ghost[ghost_count],i);
            ghost_count++;
        } else if (msg[i] == 'w') {  // player up
            m_player_d = UP;
			_m_player_p = i;
			positionSet(&m_player_p,i);
        } else if (msg[i] == 'a') {  // player left
            m_player_d = LEFT;
			_m_player_p = i;
			positionSet(&m_player_p,i);
        } else if (msg[i] == 's') {  // player down
            m_player_d = DOWN;
			_m_player_p = i;
			positionSet(&m_player_p,i);
        } else if (msg[i] == 'd') {  // player right
            m_player_d = RIGHT;
			_m_player_p = i;
			positionSet(&m_player_p,i);
        }
		//cout<<"my pos["<<m_player_p.x+1<<","<<m_player_p.y+1<<"]"<<endl;
    }
	setObjectAroundPlayer(msg);
    return 0;
}

void CalculatePath::positionSet(Pos* buff,int ID) {
	buff->x=int(ID/15);
	buff->y=int(ID%15);
}


int CalculatePath::NextMove(char* msg) {
	int result = RIGHT;
	int tempobject = 0;
	int tempmove = RIGHT;

	if((m_A_object!=0xFF)&&(m_A_object>=tempobject))
	{
		tempmove = LEFT;
		tempobject = m_A_object;

	}
	if((m_S_object!=0xFF)&&(m_S_object>=tempobject))
	{
		tempmove = DOWN;

		tempobject = m_S_object;
	}
	if((m_D_object!=0xFF)&&(m_D_object>=tempobject))
	{
		tempmove = RIGHT;
		tempobject = m_D_object;
	}
	if((m_W_object!=0xFF)&&(m_W_object>=tempobject))
	{
		tempmove = UP;
		tempobject = m_W_object;
	}
	if(0 == tempobject)
	{
		setZoneScore(msg);
		result = HighScoreZoneTowards();
	}
	else
	{
		if(m_direct_object == tempobject)
		{
			result = m_player_d;
		}
		else
		{
			result = tempmove;
		}
	}
	return	result;
}
#if 0
Pos CalculatePath::findHighestScore(char* msg){

	Pos tempPos;
	Pos findPos;
	int tempScore = 0;
	int findScore = 0;
	int radius = 1;
	int i,j = 0;
	bool findflag = false;
	for( i=-radius; i<=radius; i++ )
	{
		for( j=-radius; j<=radius; j++ )
		{
			if((( m_player_p.x+ i >= 14 )&& ( m_player_p.x + i <= 14 ))
				&&(( m_player_p.y+ j >= 14 )&& ( m_player_p.y + j <= 14 )))
			{
				//get position object
				tempScore = atoi(msg[( m_player_p.x + i ) * 15 + ( m_player_p.y + j )]);
				if((tempScore >= 0) && (tempScore <= 5) )
				{
					findflag = true;
					if(findScore >= tempScore)
					{
						findPos.x = m_player_p.x + i;
						findPos.y = m_player_p.y + j;
						findScore = tempScore;
					}
 
				}
			}
		}
	}

}
#endif


char CalculatePath::getPositionObject_Toplayer(int x, int y, char* msg)
{
	char result = ' ';
	int _pos = 0;
	if(((m_player_p.x + x)>=0&&(m_player_p.x + x)<=14)
		&&((m_player_p.y + y)>=0&&(m_player_p.y + y)<=14))
	{
		_pos = (m_player_p.x + x)*15+(m_player_p.y + y);
		result = msg[_pos];
	}
	return result;
}


int CalculatePath::IsAvailableObject(char object)
{
	int result = 0xFF;
	if((object>=0x30)&&(object<=0x35))
	{
		result = object -0x30;
	}
	return result;
}
int CalculatePath::HighScoreZoneTowards()
{
	int result = DOWN;

	if((m_highScoreZonePos.x<m_player_p.x)&&(m_W_object!=0xFF))
	{
		result = UP;
	}
	else if((m_highScoreZonePos.x>m_player_p.x)&&(m_D_object!=0xFF))
	{
		result = DOWN;
	}
	else if((m_highScoreZonePos.y<m_player_p.y)&&(m_A_object!=0xFF))
	{
		result = LEFT;
	}
	else if((m_highScoreZonePos.y>m_player_p.y)&&(m_S_object!=0xFF))
	{
		result = RIGHT;
	}
	
	return result;
}

void CalculatePath::setZoneScore(char* msg)
{
	int tmpHighScore = 0;
	Pos tmpHighScoreZonePos = {0,0};
	char tmpObject = ' ';
	int i,j;
	int m,n;
    memset(&m_zoneScore, 0, sizeof(m_zoneScore));
	//each zone
	for(i=0; i<5; i++)
	{
		for(j=0; j<5; j++)
		{
			//zone inside
			for(m=0;m<=2;m++)
			{
				for(n=0;n<=2;n++)
				{
					tmpObject = IsAvailableObject(msg[(i+m)*15+(j+n)]);
					if(0xFF != tmpObject)
					{
						m_zoneScore[i][j] += tmpObject;
					}
				}
			}
			if(m_zoneScore[i][j] >= tmpHighScore)
			{
				tmpHighScore = m_zoneScore[i][j];
				tmpHighScoreZonePos.x = i*3 +1;
				tmpHighScoreZonePos.y = j*3 +1;
			}
		}
	}
	m_highScoreZonePos.x = tmpHighScoreZonePos.x;
	m_highScoreZonePos.y = tmpHighScoreZonePos.y;
}

void CalculatePath::setObjectAroundPlayer(char* msg)
{
	m_A_object = IsAvailableObject(getPositionObject_Toplayer( 0, -1 ,msg ));
	m_S_object = IsAvailableObject(getPositionObject_Toplayer( 1, 0, msg ));
	m_D_object = IsAvailableObject(getPositionObject_Toplayer( 0, 1, msg ));
	m_W_object = IsAvailableObject(getPositionObject_Toplayer( -1, 0, msg ));

	if(m_player_d == UP)
	{
		m_direct_object = m_W_object;
	}
	else if(m_player_d == DOWN)
	{
		m_direct_object = m_S_object;
	}
	else if(m_player_d == LEFT)
	{
		m_direct_object = m_A_object;
	}
	else if(m_player_d == RIGHT)
	{
		m_direct_object = m_D_object;
	}	
	cout<<"   m_A_object:"<<m_A_object<<"   m_S_object:"<<m_S_object<<"   m_D_object:"<<m_D_object<<"   m_W_object:"<<m_W_object<<endl;

}

