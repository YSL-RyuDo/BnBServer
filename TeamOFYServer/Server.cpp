#include "Server.h"

Server::Server() : listenSocket(INVALID_SOCKET), handler_(*this) {
}

Server::~Server() {
    if (listenSocket != INVALID_SOCKET) {
        closesocket(listenSocket);
        listenSocket = INVALID_SOCKET;
    }
}


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

        char ipStr[INET_ADDRSTRLEN] = { 0 };
        InetNtopA(AF_INET, &(clientAddr.sin_addr), ipStr, INET_ADDRSTRLEN);
        int port = ntohs(clientAddr.sin_port);

        std::cout << "[접속] 클라이언트 - IP: " << ipStr << ", Port: " << port << std::endl;

        auto clientInfo = std::make_shared<ClientInfo>(clientSocket, ipStr, port);

        {
            std::lock_guard<std::mutex> lock(clientsMutex);
            clients.push_back(clientInfo);
        }

        std::thread([this, clientInfo]() {
            handler_.HandleClient(clientInfo);
            }).detach();
    }
}

void Server::RemoveClient(std::shared_ptr<ClientInfo> client) {
    {
        std::lock_guard<std::mutex> lock(clientsMutex);

        auto it = std::remove_if(clients.begin(), clients.end(),
            [&](const std::shared_ptr<ClientInfo>& c) {
                return c->socket == client->socket;
            });

        if (it != clients.end()) {
            std::cout << "[접속 종료] 클라이언트 - IP: " << client->ip << ", Port: " << client->port << std::endl;
            shutdown(client->socket, SD_BOTH);
            closesocket(client->socket);
            clients.erase(it, clients.end());
        }
    }
}
