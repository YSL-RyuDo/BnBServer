#pragma once
#include"ClientInfo.h"
#include "Utils.h"
#include "Server.h"
class ClientHandler {
public:
    void HandleClient(ClientInfo client);
    void Run();
private:
    ClientInfo client;
    Server& server_;
    //void ProcessMessage(const string& message); // º±≈√
};

