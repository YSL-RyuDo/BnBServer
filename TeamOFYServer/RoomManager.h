#pragma once
#include <vector>
#include <string>
#include <mutex>
#include <memory>
#include <sstream>
#include <unordered_map>
#include<unordered_set>
#include "Room.h"
#include "ClientInfo.h"

class Server;
class ClientHandler;
using namespace std;

class RoomManager {
private:
    Server& server_;
    ClientHandler& clientHandler_;
    vector<Room> rooms;
    mutex roomsMutex;

public:
    RoomManager(Server& server, ClientHandler& clientHandler);

    // 방 목록 전체 전송
    void BroadcastRoomlist(shared_ptr<ClientInfo> client);
    bool CreateRoom(shared_ptr<ClientInfo> client, const string& roomName, const string& mapName, const string& password);
    void BroadcastMessageExcept(SOCKET exceptSocket, const string& message);
    bool EnterRoom(shared_ptr<ClientInfo> client, const string& roomName, const string& password, string& outResponse);
    void HandleRoomChatMessage(shared_ptr<ClientInfo> sender, const string& data);

    void ExitRoom(const string& message);
    void SendRoomList(ClientInfo& client);
    void HandleCharacterChoice(ClientInfo& client, const string& data);

    bool TryStartGame(const string& roomName, std::vector<string>& usersOut);
    Room* FindRoomByName(const std::string& roomName);
    string GetGameUserListResponse(const string& roomName);
    void BroadcastToUserRoom(const std::string& senderId, const std::string& message);
    void BroadcastToRoomExcept(SOCKET excludedSocket, const std::string& message);
    const std::vector<Room>& GetRooms() const { return rooms; }

    std::unordered_map<std::string, std::unordered_set<std::string>> deadUsersByRoom;
    string GetUserRoomId(const std::string& userId);
    vector<std::string> GetUserIdsInRoom(const std::string& roomId);
    bool HandleReadyToExit(const std::string& userId, const std::string& roomId, SOCKET excludeSocket);
    std::unordered_map<std::string, std::unordered_set<std::string>> readyUsersByRoom;
};