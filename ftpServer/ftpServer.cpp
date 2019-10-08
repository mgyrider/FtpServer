#include "ftpServer.h"
#include "ftpSocket.h"
#include "json.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <cstdio>
#include <iostream>
#include <unistd.h>
#include <map>
#include <dirent.h>
using namespace std;

#define MAX_EVENTS 10
#define COMMOND_LS 0

map<int, ftpClient* > ftpClients;

int setNonBlocking(int sockfd)  
{  
    if (fcntl(sockfd, F_SETFL, fcntl(sockfd, F_GETFD, 0)|O_NONBLOCK) == -1) {  
        return -1;  
    }  
    return 0;  
}

void processLS(ftpClient *c, Json::Value& result) {
    DIR *dir = opendir(c->getCurDir());
    if (dir == NULL) {
        perror("opendir failed");
        return;
    }
    struct dirent *dirPtr;
    while ((dirPtr = readdir(dir)) != NULL) {
        Json::Value item;
        item["fileName"] = string(dirPtr->d_name);
        item["isDir"] = (dirPtr->d_type == DT_DIR ? 1 : 0);
        result.append(item);
    }
}

void processMsg(int type, int fd) {
    ftpClient* client = ftpClients[fd];
    Json::Value result;
    switch (type) {
        case COMMOND_LS:
            processLS(client, result);
            break;
        default:
            break;
    }
    /* 向客户端发送回复 */
    char buffer[10000];
    Json::Value resp;
    Json::StreamWriterBuilder wbuilder;
    resp["result"] = result;
    string jsonResp = Json::writeString(wbuilder, resp);
    int len = createMsg(COMMOND_LS, jsonResp.size(), buffer, jsonResp.c_str());
    write(fd, buffer, len);
    return;
}

int main()
{
    int serverFd, epollFd;
    struct sockaddr_in addr;
    struct epoll_event ev, events[MAX_EVENTS];

    serverFd = socket(AF_INET, SOCK_STREAM, 0);
    int tempOn = 1;
    setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &tempOn, sizeof(tempOn));
    /* 指定服务端地址 */
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8888);
    if (inet_aton("127.0.0.1", &addr.sin_addr) == 0) {
        printf("ip address error\n");
        return 1;
    }
    if (bind(serverFd, (sockaddr*)&addr, sizeof(addr)) != 0) {
        printf("bind error: %s\n", strerror(errno));
        return 1;
    }
    if (listen(serverFd, FD_SETSIZE) != 0) {
        printf("listen error: %s\n", strerror(errno));
        return 1;
    }
    cout << "监听中,端口 8888" << endl;

    /* 设置 epoll */
    epollFd = epoll_create1(0);
    if (epollFd == -1) {
        perror("epoll_create1");
        return 1;
    }
    ev.events = EPOLLIN;
    ev.data.fd = serverFd;
    if (epoll_ctl(epollFd, EPOLL_CTL_ADD, serverFd, &ev) == -1) {
        perror("epoll_ctl: listen_sock");
        return 1;
    }
    char buffer[1000];
    for ( ; ; ) {
        int nfds = epoll_wait(epollFd, events, MAX_EVENTS, -1);
        if (nfds == -1) {
            perror("epoll_wait");
            exit(EXIT_FAILURE);
        }
        int eventFd;
        for (int i = 0; i < nfds; ++i) {
            /* 如果有新连接 */
            eventFd = events[i].data.fd;
            if (eventFd == serverFd) {
                int clientFd = accept(serverFd, NULL, NULL);
                if (clientFd == -1) {
                    perror("accpet");
                    return 1;
                }
                /* 监听客户, 边缘触发,需要设置为非阻塞模式 */
                setNonBlocking(clientFd);
                ev.events = EPOLLIN | EPOLLET;
                ev.data.fd = clientFd;
                if (epoll_ctl(epollFd, EPOLL_CTL_ADD, clientFd, &ev) == -1) {
                    perror("epoll_ctl: client error");
                    return 1;
                }
                ftpClients[clientFd] = new ftpClient(clientFd);
            } else {
                int type = readMsg(eventFd, buffer);
                if (type != -1) {
                    processMsg(type, eventFd);
                }
            }
        }
    }
    
    return 0;
}
