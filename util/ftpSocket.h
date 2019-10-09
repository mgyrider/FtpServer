#ifndef __FTP_SOCKET_H__
#define __FTP_SOCKET_H__
#include "json.h"
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>

#define COMMOND_LS 0
#define COMMOND_GET 1

int createMsg(int type, int len, char *data, const char *msg);
int readMsg(int eventFd, char *buffer);
int getFileSize(const char *path);
void sendJson(int fd, Json::Value& resp, char *buffer);
int parseJson(char* buffer, Json::Value& resp);

#endif
