#include "Server.h"

Server::Server()
    : roomManager_(*this),
    userManager_(*this),
    handler_(*this, userManager_, roomManager_),
    listenSocket(INVALID_SOCKET)
{
    userManager_.SetClientHandler(&handler_);
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
        cout << "WSAStartup 실패: " << result << endl;
        return false;
    }

    listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenSocket == INVALID_SOCKET) {
        cout << "소켓 생성 실패" << endl;
        WSACleanup();
        return false;
    }

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port);

    if (bind(listenSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        cout << "바인드 실패" << endl;
        closesocket(listenSocket);
        WSACleanup();
        return false;
    }

    if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR) {
        cout << "리스닝 실패" << endl;
        closesocket(listenSocket);
        WSACleanup();
        return false;
    }

    cout << "서버 초기화 성공, 포트: " << port << endl;
    return true;
}

void Server::Run() {
    auto loadedUsers = userManager_.LoadUsers("users.csv");
    if (loadedUsers.empty()) {
        cerr << "users.csv 로드 실패 또는 내용 없음" << endl;
        return;
    }
    cout << "서버 실행 중..." << endl;
    AcceptClients();
}

void Server::AcceptClients() {
    while (true) {
        sockaddr_in clientAddr;
        int clientAddrSize = sizeof(clientAddr);
        SOCKET clientSocket = accept(listenSocket, (sockaddr*)&clientAddr, &clientAddrSize);
        if (clientSocket == INVALID_SOCKET) {
            cout << "클라이언트 접속 실패" << endl;
            continue;
        }

        char ipStr[INET_ADDRSTRLEN] = { 0 };
        InetNtopA(AF_INET, &(clientAddr.sin_addr), ipStr, INET_ADDRSTRLEN);
        int port = ntohs(clientAddr.sin_port);

        cout << "[접속] 클라이언트 - IP: " << ipStr << ", Port: " << port << endl;

        auto clientInfo = make_shared<ClientInfo>(clientSocket, ipStr, port);

        {
            lock_guard<mutex> lock(clientsMutex);
            clients.push_back(clientInfo);
        }

        thread([this, clientInfo]() {
            handler_.HandleClient(clientInfo);
            }).detach();
    }
}

void Server::RemoveClient(shared_ptr<ClientInfo> client) {
    {
        lock_guard<mutex> lock(clientsMutex);

        auto it = remove_if(clients.begin(), clients.end(),
            [&](const shared_ptr<ClientInfo>& c) {
                return c->socket == client->socket;
            });

        if (it != clients.end()) {
            cout << "[접속 종료] 클라이언트 - IP: " << client->ip << ", Port: " << client->port << endl;
            shutdown(client->socket, SD_BOTH);
            closesocket(client->socket);
            clients.erase(it, clients.end());
        }
    }
}

vector<shared_ptr<ClientInfo>>& Server::GetClients() { return clients; }

mutex& Server::GetClientsMutex() { return clientsMutex; }