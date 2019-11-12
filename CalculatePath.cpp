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
#include <list>

using namespace std;

enum DIRECTION {
    UP = 0,
    DOWN,
    LEFT,
    RIGHT
};


CalculatePath::CalculatePath(int sockfd): m_socketfd(sockfd) {
    startThread();
    for(int i=0; i<225; i++) {
        m_move_flag[i] = false;
    }
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
    float a_score = 0.0f;
    float s_score = 0.0f;
    float d_score = 0.0f;
    float w_score = 0.0f;
    int temp_dir = -1;
    int temp_score = -1;
    int move_dir = -1;

    if ((m_A_object!=0xFF) && !maybeGhost(LEFT)) {
        a_score = getPathScore(LEFT, msg);
    }
    if ((m_S_object!=0xFF) && !maybeGhost(DOWN)) {
        s_score = getPathScore(DOWN, msg);
    }
    if ((m_D_object!=0xFF) && !maybeGhost(RIGHT)) {
        d_score = getPathScore(RIGHT, msg);
    }
    if ((m_W_object!=0xFF) && !maybeGhost(UP)) {
        w_score = getPathScore(UP, msg);
    }

    list<float> score_list = {a_score, s_score, d_score, w_score};
    score_list.sort(); 
    list<float>::iterator it;
    cout << "score_list contains:";
    for (it=score_list.begin(); it!=score_list.end(); ++it) {
      cout << ' ' << *it;
    }
    cout << '\n';
    float highest_score = score_list.back();
    if (highest_score != 0.0f) {
        if (highest_score == a_score) {
            temp_score = LEFT;
            if (m_player_d == LEFT) {
                temp_dir = LEFT;
            }
        }
        if (highest_score == s_score) {
            temp_score = DOWN;
            if (m_player_d == DOWN) {
                temp_dir = DOWN;
            }
        }
        if (highest_score == d_score) {
            temp_score = RIGHT;
            if (m_player_d == RIGHT) {
                temp_dir = RIGHT;
            }
        }
        if (highest_score == w_score) {
            temp_score = UP;
            if (m_player_d == UP) {
                temp_dir = UP;
            }
        }
        if (temp_dir != -1) {
            move_dir = temp_dir;
        } else {
            move_dir = temp_score;
        }
    } else {
        move_dir = 0;
    }
    return move_dir;
}

float CalculatePath::getPathScore(int dir, char* msg) {
    int score=0;
    int count = 0;
    int temp_x = m_player_p.x;
    int temp_y = m_player_p.y;
    int isAvailable = 0xFF;
    if (m_player_d != dir) {
        count++;
    }
    if (dir == LEFT) {
        while (temp_y>0) {
            char object = getPositionObject(temp_x, temp_y-1, msg);
            isAvailable = IsAvailableObject(object);
            if (isAvailable != 0xFF) {
                score += object - '0';
                count++;
                temp_y--;
            } else {
                break;
            }
        }
        cout<<"left path score is "<<score<<endl;
    } else if (dir == DOWN) {
        while (temp_x<15) {
            char object = getPositionObject(temp_x+1, temp_y, msg);
            isAvailable = IsAvailableObject(object);
            if (isAvailable != 0xFF) {
                score += object - '0';
                count++;
                temp_x++;
            } else {
                break;
            }
        }
        cout<<"down path score is "<<score<<endl;
    } else if (dir == RIGHT) {
        while (temp_y<15) {
            char object = getPositionObject(temp_x, temp_y+1, msg);
            isAvailable = IsAvailableObject(object);
            if (isAvailable != 0xFF) {
                score += object - '0';
                count++;
                temp_y++;
            } else {
                break;
            }
        }
        cout<<"right path score is "<<score<<endl;
    } else if (dir == UP) {
        while (temp_x>0) {
            char object = getPositionObject(temp_x-1, temp_y, msg);
            isAvailable = IsAvailableObject(object);
            if (isAvailable != 0xFF) {
                score += object - '0';
                count++;
                temp_x--;
            } else {
                break;
            }
        }
        cout<<"up path score is "<<score<<endl;
    } else {
    }
    if (count != 0) return ((float)score/(float)count);
    return 0.0f;
}

bool CalculatePath::maybeGhost(int dir) {
    bool result=false;
    list<Pos> ghost;
    Pos player;

    for(int i=0; i<3; i++) {
        Pos temp;
        temp.x = m_ghost[i].x;
        temp.y = m_ghost[i].y;
        ghost.push_back(temp);
        
        temp.x = m_ghost[i].x-1;
        temp.y = m_ghost[i].y;
        ghost.push_back(temp);
        
        temp.x = m_ghost[i].x+1;
        temp.y = m_ghost[i].y;
        ghost.push_back(temp);
        
        temp.x = m_ghost[i].x;
        temp.y = m_ghost[i].y-1;
        ghost.push_back(temp);
        
        temp.x = m_ghost[i].x;
        temp.y = m_ghost[i].y+1;
        ghost.push_back(temp);
    }

    if (dir != m_player_d) {
        player.x = m_player_p.x;
        player.y = m_player_p.y;
    } else {
        if (dir == UP) {
            player.x = m_player_p.x - 1;
            player.y = m_player_p.y;
        } else if (dir == DOWN) {
            player.x = m_player_p.x + 1;
            player.y = m_player_p.y;
        } else if (dir == LEFT) {
            player.x = m_player_p.x;
            player.y = m_player_p.y - 1;
        } else if (dir == RIGHT) {
            player.x = m_player_p.x;
            player.y = m_player_p.y + 1;
        } else {
        }
    }

    list<Pos>::iterator it;
    for (it=ghost.begin(); it!=ghost.end(); ++it) {
        if ((it->x == player.x) && (it->y == player.y)) {
            result = true;
            break;
        }
    }
    cout<<dir<<" maybeGhost is "<<result<<endl;
    return result;
}

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

char CalculatePath::getPositionObject(int x, int y, char* msg)
{
    char result = ' ';
    int _pos = 0;
    if (x>=0 && x<=14 && y>=0 && y<=14) {
        _pos = x*15+y;
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

void CalculatePath::setObjectAroundPlayer(char* msg)
{
    m_A_object = IsAvailableObject(getPositionObject_Toplayer( 0, -1 ,msg ));
    m_S_object = IsAvailableObject(getPositionObject_Toplayer( 1, 0, msg ));
    m_D_object = IsAvailableObject(getPositionObject_Toplayer( 0, 1, msg ));
    m_W_object = IsAvailableObject(getPositionObject_Toplayer( -1, 0, msg ));

    cout<<"   m_A_object:"<<m_A_object<<"   m_S_object:"<<m_S_object<<"   m_D_object:"<<m_D_object<<"   m_W_object:"<<m_W_object<<endl;

}

