#ifndef __FTP_SERVER_H__
#define __FTP_SERVER_H__
#include <string>
#include <map>
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
private:
    int fd;
    std::string curDir;
    map<string, bool> FileList;
};

#endif
