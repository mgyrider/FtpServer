#include "ftpSocket.h"
#include <sys/stat.h>
#include <iostream>
using namespace std;

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
    if (res <= 0) {
        return -1;
    }
    type = ntohl(type);
    /* 数据段长度 */
    read(eventFd, &len, sizeof(len));
    len = ntohl(len);
    read(eventFd, buffer, len);
    buffer[len] = '\0';
    //printf("%s\n", buffer);
    return type;
}

int getFileSize(const char *path)
{
    unsigned long filesize = -1;	
	struct stat statbuff;
	if(stat(path, &statbuff) < 0){
		return filesize;
	}else{
		filesize = statbuff.st_size;
	}
	return filesize;
}

void sendJson(int fd,Json::Value& resp, char *buffer)
{
    static Json::StreamWriterBuilder wbuilder;
    string jsonResp = Json::writeString(wbuilder, resp);
    int len = createMsg(COMMOND_LS, jsonResp.size(), buffer, jsonResp.c_str());
    write(fd, buffer, len);
}

int parseJson(char* buffer, Json::Value& resp)
{
    static Json::Reader reader;
    if (!reader.parse(buffer, resp, false)) {
        return -1;
    }
    return 0;
}