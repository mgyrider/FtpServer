#ifndef __FTP_SERVER_H__
#define __FTP_SERVER_H__
#include <string>
#include <map>
#include "json.h"
using namespace std;

class ftpClient
{
public:
    ftpClient(int fd):fd(fd), curDir(".") {
        
    }
    const char *getCurDir()
    {
        return curDir.c_str();
    }
    void addFile(string s, bool isDir) {
        FileList[s] = isDir;
    }
    void clearFileList() {
        FileList.clear();
    }
    Json::Value& getJsonReq() {
        return jsonRequest;
    }
    int getFd() {
        return fd;
    }
private:
    int fd;
    std::string curDir;
    map<string, bool> FileList;
    Json::Value jsonRequest;
};

#endif
