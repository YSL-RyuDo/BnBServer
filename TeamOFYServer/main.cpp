#pragma comment(lib, "ws2_32.lib")
#include "Server.h"


int main() {
    Server server;
    if (!server.Initialize(9000)) {
        cout << "서버 초기화 실패" << endl;
        return 1;
    }

    server.Run();
    return 0;
}