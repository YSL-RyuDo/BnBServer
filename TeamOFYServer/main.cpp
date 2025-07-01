#pragma comment(lib, "ws2_32.lib")
#include "Server.h"
#include <iostream>
#include <direct.h> // Windows ����

void PrintWorkingDirectory() {
    char buffer[512];
    _getcwd(buffer, sizeof(buffer));
    std::cout << "���� �۾� ���͸�: " << buffer << std::endl;
}


int main() {
    //PrintWorkingDirectory();

    Server server;
    if (!server.Initialize(9000)) {
        cout << "���� �ʱ�ȭ ����" << endl;
        return 1;
    }

    server.Run();
    return 0;
}