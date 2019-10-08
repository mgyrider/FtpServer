#ifndef __FTP_SOCKET_H__
#define __FTP_SOCKET_H__

#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>


int createMsg(int type, int len, char *data, const char *msg);
int readMsg(int eventFd, char *buffer);

#endif
