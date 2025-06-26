#pragma once
#include <winsock2.h> 
#include <ws2tcpip.h>
#include <vector>
#include <mutex>
#include "ClientInfo.h"
#include "Utils.h"
class Server {
public:
    bool Initialize(unsigned short port);
    void RemoveClient(const ClientInfo& client);
    void Run();
    void AcceptClients();

private:
    SOCKET listenSocket;
    vector<ClientInfo> clients;
    mutex clientsMutex;
};