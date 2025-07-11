#pragma once
#include<iostream>
#include<unordered_map>
#include <string>
#include"ClientInfo.h"
#include "Utils.h"
#include "UserManager.h"
#include "RoomManager.h"

class Server;
class ClientHandler {
public:
    ClientHandler(Server& server, UserManager& userManager, RoomManager& roomManager);
    ~ClientHandler();
    void HandleClient(shared_ptr<ClientInfo>);

    unordered_map<string, shared_ptr<ClientInfo>>& GetClientsMap() {
        return clientsMap;
    }
    //vector<shared_ptr<ClientInfo>> clients;
private:
    Server& server_;
    UserManager& userManager_;
    RoomManager& roomManager_;
    
    unordered_map<string, shared_ptr<ClientInfo>> clientsMap;
    void ProcessMessages(shared_ptr<ClientInfo>, const string&);

    bool SendToClient(std::shared_ptr<ClientInfo> client, const std::string& response);
};

