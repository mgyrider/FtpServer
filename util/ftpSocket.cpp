#include "ftpSocket.h"

int createMsg(int type, int len, char *data, const char *msg)
{
    type = htonl(type);
    memcpy(data, &type, sizeof(type));
    int tempLen = htonl(len);
    memcpy(data + sizeof(type), &tempLen, sizeof(tempLen));
    memcpy(data + 2 * sizeof(int), msg, len);
    return len + 2 * sizeof(int);
}

int readMsg(int eventFd, char *buffer)
{
    int type, len;
    int res = read(eventFd, &type, sizeof(type));
    /* 客户端连接关闭 */
    if (res == 0) {
        return -1;
    }
    type = ntohs(type);
    /* 数据段长度 */
    read(eventFd, &len, sizeof(len));
    len = ntohl(len);
    read(eventFd, buffer, len);
    buffer[len] = '\0';
    //printf("%s\n", buffer);
    return type;
}
