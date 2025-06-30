#pragma once
#include<iostream>
#include"ClientInfo.h"
#include "Utils.h"

class Server;
class ClientHandler {
public:
    ClientHandler(Server&);
    ~ClientHandler();
    void HandleClient(shared_ptr<ClientInfo>);
private:
    Server& server_;
    void ProcessMessages(shared_ptr<ClientInfo>, const string&);
};

