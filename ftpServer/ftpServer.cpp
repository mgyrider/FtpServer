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
#include <sys/sendfile.h>
using namespace std;

#define MAX_EVENTS 10

map<int, ftpClient* > ftpClients;

int setNonBlocking(int sockfd)  
{  
    if (fcntl(sockfd, F_SETFL, fcntl(sockfd, F_GETFD, 0)|O_NONBLOCK) == -1) {  
        return -1;  
    }  
    return 0;  
}

int processLs(ftpClient *c, Json::Value& resp) {
    DIR *dir = opendir(c->getCurDir());
    if (dir == NULL) {
        perror("opendir failed");
        return 1;
    }
    Json::Value result;
    struct dirent *dirPtr;
    while ((dirPtr = readdir(dir)) != NULL) {
        Json::Value item;
        item["fileName"] = string(dirPtr->d_name);
        item["isDir"] = (dirPtr->d_type == DT_DIR ? 1 : 0);
        result.append(item);
    }
    resp["retcode"] = 0;
    resp["result"] = result;
    char buffer[10000];
    sendJson(c->getFd(), resp, buffer);
    return 0;
}

int processGet(ftpClient *c, Json::Value& resp) {
    string fileName = c->getJsonReq()["fileName"].asString();
    int fileFd = open(fileName.c_str(), O_RDONLY);
    if (fileFd == -1) {
        resp["error"] = "file does not exist";
        return 1;
    }
    int fileSize = getFileSize(fileName.c_str());
    if (fileSize == -1) {
        return 1;
    }
    resp["retcode"] = 0;
    resp["fileSize"] = fileSize;
    char buffer[10000];
    sendJson(c->getFd(), resp, buffer);
    /* 发送文件 */
    int res = sendfile(c->getFd(), fileFd, 0, fileSize);
    cout << "sendres " << res << " " << endl;
    return 0;
}

void processMsg(int type, int fd) {
    ftpClient* client = ftpClients[fd];
    Json::Value resp;
    int retcode = 0;
    switch (type) {
        case COMMOND_LS:
            retcode = processLs(client, resp);
            break;
        case COMMOND_GET:
            retcode = processGet(client, resp);
            break;
        default:
            break;
    }
    /* 向客户端发送回复 */
    if (retcode != 0) {
        resp["retcode"] = retcode;
        char buffer[10000];
        sendJson(fd, resp, buffer);
    }
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
                // cout << type << endl;
                if (type != -1) {
                    if (parseJson(buffer, ftpClients[eventFd]->getJsonReq()) != 0) {
                        cout << "json 格式错误" << endl;
                        continue;
                    }
                    processMsg(type, eventFd);
                } else {
                    /* 关闭连接 */
                    printf("客户端 %d 关闭\n", eventFd);
                    ftpClient* c = ftpClients[eventFd];
                    if (c) {
                        delete c;
                    }
                    ftpClients.erase(eventFd);
                    close(eventFd);
                }
            }
        }
    }
    
    return 0;
}
