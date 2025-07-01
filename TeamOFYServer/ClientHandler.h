#pragma once
#include<iostream>
#include<unordered_map>
#include"ClientInfo.h"
#include "Utils.h"
#include "UserManager.h"

class Server;
class ClientHandler {
public:
    ClientHandler(Server& server, UserManager& userManager);
    ~ClientHandler();
    void HandleClient(shared_ptr<ClientInfo>);
private:
    Server& server_;
    UserManager& userManager_;
    vector<shared_ptr<ClientInfo>> clients;
    unordered_map<string, shared_ptr<ClientInfo>> clientsMap;
    void ProcessMessages(shared_ptr<ClientInfo>, const string&);

    bool SendToClient(std::shared_ptr<ClientInfo> client, const std::string& response);
};

