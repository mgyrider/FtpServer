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

vector<string> readCommand()
{
    string str;
    vector<string> vec;
    if (!getline(cin, str)) {
        vec.push_back("EOF"); /* 读到EOF */
        return vec;
    }
    int i = 0;
    int len = str.size();
    while (i < len && str[i] == ' ') {
        ++i;
    }
    if (i == len) {
        return vec;
    }
    int j = len - 1;
    while (j >= 0 && str[j] == ' ') {
        --j;
    }
    int k = i;
    for (k = i; k <= j; ++k) {
        if (str[k] == ' ') {
            int len2 = k - i;
            vec.push_back(str.substr(i, len2));
            while (k <= j && str[k] == ' ') {
                ++k;
            }
            i = k;
            --k;
        }
    }
    vec.push_back(str.substr(i, j - i + 1));
    // for (auto s: vec) {
    //     cout << s << endl;
    // }
    return vec;
}

void clientProcessLs(int fd, char* buffer)
{
    string s = "{}";
    int len = createMsg(COMMOND_LS, s.size(), buffer, s.c_str());
    write(fd, buffer, len);
    readMsg(fd, buffer);
    /* 解析JSON */
    Json::Value  resp;
    if (parseJson(buffer, resp) != 0) {
        cout << "json 格式错误" << endl;
        return;
    }
    Json::Value jresult = resp["result"];
    /* 遍历文件目录 */
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

void clientProcessGet(int fd, char* buffer, string fileName, string desPath = "/home/") {
    Json::Value req;
    req["fileName"] = fileName;
    Json::StreamWriterBuilder wbuilder;
    string jsonReq = Json::writeString(wbuilder, req);
    int len = createMsg(COMMOND_GET, jsonReq.size(), buffer, jsonReq.c_str());
    write(fd, buffer, len);
    readMsg(fd, buffer);
    /* 解析JSON */
    Json::Value  resp;
    if (parseJson(buffer, resp) != 0) {
        cout << "json 格式错误" << endl;
        return;
    }
    int retcode = resp["retcode"].asInt();
    if (retcode == 0) {
        long long fileSize =  resp["fileSize"].asInt64();
        cout << "get file size " << fileSize << endl;
        /* 开始接收文件 */
        char buf[1000];
        int n = read(fd, buf, fileSize);
        cout << n << endl;
        buf[n] = '\0';
        cout << buf << endl;
    } else {
        cout << resp["error"].asString() << endl;
    }
}

int userShell(int fd)
{
    char buffer[1000];
    for ( ; ; ) {
        printf("\n\n");
        printf("# ");
        vector<string> cmdline =  readCommand();
        if (cmdline.size() == 0) {
            continue;
        }
        // cout << cmdline[0] << " " << cmdline.size() << endl;
        if (cmdline[0] == "ls") {
            clientProcessLs(fd, buffer);
        } else if (cmdline[0] == "cd") {
            cout << "cd" << endl;
        } else if (cmdline[0] == "get" && cmdline.size() == 2) {
            clientProcessGet(fd, buffer, cmdline[1]);
        }
        /* EOF */
        if (cmdline[0] == "EOF") {
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
