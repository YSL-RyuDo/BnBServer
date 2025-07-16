#pragma once
#include<iostream>
#include<unordered_map>
#include <string>
#include <numeric>
#include <random>
#include"ClientInfo.h"
#include "Utils.h"
#include "UserManager.h"
#include "RoomManager.h"
#include "MapManager.h"
#include "Player.h"
#include "CharacterStatsManager.h"

class Server;
class ClientHandler {
public:
    ClientHandler(Server& server, UserManager& userManager, RoomManager& roomManager, MapManager& mapManager, Player& player);
    ~ClientHandler();
    void HandleClient(shared_ptr<ClientInfo>);

    
    unordered_map<string, shared_ptr<ClientInfo>>& GetClientsMap() {
        return clientsMap;
    }

    string GetNicknameById(const std::string& id) const {
        for (const auto& pair : clientsMap) {
            if (pair.second && pair.second->id == id) {
                return pair.second->nickname;
            }
        }
        return id;
    }

    string GetIdByNickname(const std::string& nickname) const {
        for (const auto& pair : clientsMap) {
            if (pair.second && pair.second->nickname == nickname) {
                return pair.second->id;
            }
        }
        return "";  // 닉네임을 찾지 못했을 때 빈 문자열 반환
    }

    //vector<shared_ptr<ClientInfo>> clients;
private:
    Server& server_;
    UserManager& userManager_;
    RoomManager& roomManager_;
    MapManager& mapManager_;
    Player& player_;
    CharacterStatsManager characterStatsManager_;
    unordered_map<string, shared_ptr<ClientInfo>> clientsMap;

    void ProcessMessages(shared_ptr<ClientInfo>, const string&);

    bool SendToClient(std::shared_ptr<ClientInfo> client, const std::string& response);
};

