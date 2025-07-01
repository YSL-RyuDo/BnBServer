#pragma comment(lib, "ws2_32.lib")
#include "Server.h"
#include <iostream>
#include <direct.h> // Windows 전용

void PrintWorkingDirectory() {
    char buffer[512];
    _getcwd(buffer, sizeof(buffer));
    std::cout << "현재 작업 디렉터리: " << buffer << std::endl;
}


int main() {
    //PrintWorkingDirectory();

    Server server;
    if (!server.Initialize(9000)) {
        cout << "서버 초기화 실패" << endl;
        return 1;
    }

    server.Run();
    return 0;
}