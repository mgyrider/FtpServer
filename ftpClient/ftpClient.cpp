#include "ftpSocket.h"
#include "json.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <string.h>
#include <cstdio>
#include <iostream>
#include <unistd.h>
#include <algorithm>
#include <vector>
using namespace std;

class FileItem
{
public:
    FileItem(string s, bool isDir):fileName(s),isDir(isDir){ 
    }
    static bool sortCmp(FileItem a, FileItem b) {
        if (a.isDir != b.isDir) {
            return a.isDir > b.isDir;
        } else {
            return a.fileName < b.fileName;
        }
    }
    string getFileName() {
        return fileName;
    }
    bool isDirectory() {
        return isDir;
    }
private:
    string fileName;
    bool isDir;
};


void clientProcessLs(int fd, char* buffer)
{
    string s = "{}";
    int len = createMsg(0, s.size(), buffer, s.c_str());
    write(fd, buffer, len);
    readMsg(fd, buffer);
    /* 解析JSON */
    Json::Reader reader;
    Json::Value  resp;
    if (!reader.parse(buffer, resp, false)) {
        cout << "json 格式错误" << endl;
        return;
    }
    Json::Value jresult = resp["result"];
    vector<FileItem> FileList;
    for (int i = 0; i < jresult.size(); ++i) {
        Json::Value& item = jresult[i];
        string fileName = item["fileName"].asString();
        bool isDir = item["isDir"].asBool();
        FileList.push_back(FileItem(fileName, isDir));
    }
    sort(FileList.begin(), FileList.end(), FileItem::sortCmp);
    for (auto& item : FileList) {
        printf("%-20s\t%s\n", item.getFileName().c_str(), item.isDirectory()?"Dir":"File");
    }
}

int userShell(int fd)
{
    char buffer[1000];
    for ( ; ; ) {
        string cmdline;
        printf("\n\n");
        printf("# ");
        cin >> cmdline;
        if (cmdline == "ls") {
            clientProcessLs(fd, buffer);
        } else if (cmdline == "cd") {
            cout << "cd" << endl;
        } else {
            cout << "error\n";
        }
        /* EOF */
        if (cmdline.size() == 0) {
            break;
        }
    }
}

int main()
{
    int fd;
    struct sockaddr_in addr;
    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1) {
        printf("%s", strerror(errno));
        return 0;
    }
    /* 指定服务端地址 */
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8888);
    if (inet_aton("127.0.0.1", &addr.sin_addr) == 0) {
        printf("ip address error\n");
        return 0;
    }
    /* 连接服务端 */
    if (connect(fd, (sockaddr*)&addr, sizeof(addr)) != 0) {
        printf("connect error :%s\n", strerror(errno));
        return 0;
    } else {
        cout << "连接服务端成功" << endl;
    }
    userShell(fd);
    return 0;
}
