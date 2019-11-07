#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "CalculatePath.h"

#define RECV_MAXSIZE 256
#define SEND_MAXSIZE 3
#define SERVER_PORT 9527
#define KEY "(0e6b3caf0e9a4e2baadbd1c2aa858a2b)"

int main(int argc, char** argv){
    int   sockfd;
    char  recvbuf[RECV_MAXSIZE];
    struct sockaddr_in  servaddr;

    if (argc != 2){
        printf("usage: ./client <ipaddress>\n");
        return 0;
    }

    // create socket
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        printf("create socket error: %s(errno: %d)\n", strerror(errno),errno);
        return -1;
    }

    // create CalculatePath refrence
    CalculatePath* calculatePath = new CalculatePath(sockfd);

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(SERVER_PORT);
    if (inet_pton(AF_INET, argv[1], &servaddr.sin_addr) <= 0){
        printf("inet_pton error for %s\n",argv[1]);
        return -1;
    }
	printf("1111\r\n");
    // connect to server
    if (connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0){
        printf("connect error: %s(errno: %d)\n",strerror(errno),errno);
        return -1;
    }

    // register key
    printf("send key to server \n");
    memset(recvbuf, 0, RECV_MAXSIZE);
    if (send(sockfd, KEY, strlen(KEY), 0) < 0){
        printf("send key error: %s(errno: %d)\n", strerror(errno), errno);
        return -1;
    }
    if (recv(sockfd, &recvbuf, sizeof(recvbuf), 0) < 0) {
        printf("receive msg error: %s(errno: %d)\n", strerror(errno), errno);
        return -1;
    } else {
        printf("receive %s, start game!\n", recvbuf);
    }

    // cycle receive msg from server
    while (true) {
        int size = recv(sockfd, &recvbuf, sizeof(recvbuf), 0);
        if(size < 0) {
            printf("receive msg error: %s(errno: %d)\n", strerror(errno), errno);
            continue;
        } else if (size == 2) {
            printf("receive OK!\n");
            continue;
        } else if (size == 9) {
            printf("receive GAME OVER!\n");
            break;
        } else {
            printf("receive %s, size is %d\n", recvbuf, size);
            // send recvbuf to dispatch element
            calculatePath->dispatch(recvbuf, size);
        }
    }

    close(sockfd);
    return 0;
}

