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
        std::cout << "WSAStartup ����: " << result << std::endl;
        return false;
    }

    listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenSocket == INVALID_SOCKET) {
        std::cout << "���� ���� ����" << std::endl;
        WSACleanup();
        return false;
    }

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port);

    if (bind(listenSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cout << "���ε� ����" << std::endl;
        closesocket(listenSocket);
        WSACleanup();
        return false;
    }

    if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR) {
        std::cout << "������ ����" << std::endl;
        closesocket(listenSocket);
        WSACleanup();
        return false;
    }

    std::cout << "���� �ʱ�ȭ ����, ��Ʈ: " << port << std::endl;
    return true;
}

void Server::Run() {
    std::cout << "���� ���� ��..." << std::endl;
    AcceptClients();
}

void Server::AcceptClients() {
    while (true) {
        sockaddr_in clientAddr;
        int clientAddrSize = sizeof(clientAddr);
        SOCKET clientSocket = accept(listenSocket, (sockaddr*)&clientAddr, &clientAddrSize);
        if (clientSocket == INVALID_SOCKET) {
            std::cout << "Ŭ���̾�Ʈ ���� ����" << std::endl;
            continue;
        }

        char ipStr[INET_ADDRSTRLEN] = { 0 };
        InetNtopA(AF_INET, &(clientAddr.sin_addr), ipStr, INET_ADDRSTRLEN);
        int port = ntohs(clientAddr.sin_port);

        std::cout << "[����] Ŭ���̾�Ʈ - IP: " << ipStr << ", Port: " << port << std::endl;

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
            std::cout << "[���� ����] Ŭ���̾�Ʈ - IP: " << client->ip << ", Port: " << client->port << std::endl;
            shutdown(client->socket, SD_BOTH);
            closesocket(client->socket);
            clients.erase(it, clients.end());
        }
    }
}
