#pragma once
#include<iostream>
#include<unordered_map>
#include <string>
#include <numeric>
#include <random>
#include <unordered_set>

#include"ClientInfo.h"
#include "Utils.h"
#include "UserManager.h"
#include "RoomManager.h"
#include "MapManager.h"
#include "Player.h"
#include "CharacterStatsManager.h"
using namespace std;
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
        string trimmedID = Trim(id);
        for (const auto& pair : clientsMap) {
            if (pair.second && pair.second->id == trimmedID) {
                return pair.second->nickname;
            }
        }
        return id;
    }

    string GetIdByNickname(const string& nickname) const {
        string trimmedNick = Trim(nickname);
        for (const auto& pair : clientsMap) {
            if (pair.second && Trim(pair.second->nickname) == trimmedNick) {
                return pair.second->id;
            }
        }
        return "";
    }

    bool GetUserPositionById(const std::string& userId, std::pair<float, float>& outPos);

    void SendSetInfo(std::shared_ptr<ClientInfo> client, const std::string& nickname, const UserProfile& profile);
    void SendWinRate(std::shared_ptr<ClientInfo> client, const std::string& nickname, const UserWinLossStats& stats);
    void SendUserEmotes(std::shared_ptr<ClientInfo> client, const std::string& nickname);
    void SendUserBallons(std::shared_ptr<ClientInfo> client, const std::string& nickname);
    void SendUserIcons(std::shared_ptr<ClientInfo> client, const std::string& nickname);

    void OnClientDisconnected(shared_ptr<ClientInfo> client);
    void RemoveLoginSession(const std::string& userId);
private:
    Server& server_;
    UserManager& userManager_;
    RoomManager& roomManager_;
    MapManager& mapManager_;
    Player& player_;
    CharacterStatsManager characterStatsManager_;
    unordered_map<string, shared_ptr<ClientInfo>> clientsMap;
    unordered_map<std::string, std::pair<float, float>> userPositions;
    void ProcessMessages(shared_ptr<ClientInfo>, const string&);

    bool SendToClient(std::shared_ptr<ClientInfo> client, const std::string& response);
};

