#include "Server.h"
bool Server::Initialize(unsigned short port) {
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        std::cout << "WSAStartup 실패: " << result << std::endl;
        return false;
    }

    listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenSocket == INVALID_SOCKET) {
        std::cout << "소켓 생성 실패" << std::endl;
        WSACleanup();
        return false;
    }

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port);

    if (bind(listenSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cout << "바인드 실패" << std::endl;
        closesocket(listenSocket);
        WSACleanup();
        return false;
    }

    if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR) {
        std::cout << "리스닝 실패" << std::endl;
        closesocket(listenSocket);
        WSACleanup();
        return false;
    }

    std::cout << "서버 초기화 성공, 포트: " << port << std::endl;
    return true;
}

void Server::Run() {
    std::cout << "서버 실행 중..." << std::endl;
    AcceptClients();
}

void Server::AcceptClients() {
    while (true) {
        sockaddr_in clientAddr;
        int clientAddrSize = sizeof(clientAddr);
        SOCKET clientSocket = accept(listenSocket, (sockaddr*)&clientAddr, &clientAddrSize);
        if (clientSocket == INVALID_SOCKET) {
            std::cout << "클라이언트 접속 실패" << std::endl;
            continue;
        }

        // IP 주소 문자열 변환
        char ipStr[INET_ADDRSTRLEN] = { 0 };
        InetNtopA(AF_INET, &(clientAddr.sin_addr), ipStr, INET_ADDRSTRLEN);

        // 포트 번호 변환 (네트워크 바이트 순서 -> 호스트 바이트 순서)
        int port = ntohs(clientAddr.sin_port);

        std::cout << "클라이언트 접속됨 - IP: " << ipStr << ", Port: " << port << std::endl;
    }
}

void Server::RemoveClient(const ClientInfo& client) {
    lock_guard<mutex> lock(clientsMutex);
    clients.erase(
        remove_if(clients.begin(), clients.end(),
            [&](const ClientInfo& c) { return c.socket == client.socket; }),
        clients.end());
}