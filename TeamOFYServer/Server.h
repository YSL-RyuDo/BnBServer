#pragma once
#include <winsock2.h> 
#include <ws2tcpip.h>
#include <vector>
#include <mutex>
#include "ClientInfo.h"
#include "ClientHandler.h"
#include "Utils.h"
#include "UserManager.h"

class Server {
public:
    Server();
    ~Server();

    bool Initialize(unsigned short);
    void RemoveClient(shared_ptr<ClientInfo>);
    void Run();
    void AcceptClients();
    mutex clientsMutex;
private:
    SOCKET listenSocket;
    vector<shared_ptr<ClientInfo>> clients;
    

    ClientHandler handler_;
    UserManager userManager_;
};