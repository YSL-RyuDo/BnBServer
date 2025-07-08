#pragma once
#include <vector>
#include <string>
#include <mutex>
#include <memory>
#include "Room.h"
#include "ClientInfo.h"

class Server;
class ClientHandler;
using namespace std;

class RoomManager {
public:
    RoomManager(Server& server, ClientHandler& clientHandler);

    // 방 목록 전체 전송
    void BroadcastRoomlist(shared_ptr<ClientInfo> client);
    bool CreateRoom(shared_ptr<ClientInfo> client, const string& roomName, const string& mapName, const string& password);
    void BroadcastMessageExcept(SOCKET exceptSocket, const string& message);
    bool EnterRoom(shared_ptr<ClientInfo> client, const string& roomName, const string& password, string& outResponse);
    void HandleRoomChatMessage(shared_ptr<ClientInfo> sender, const string& data);

    void ExitRoom(const std::string& message);
    void SendRoomList(ClientInfo& client);
private:
    Server& server_;
    ClientHandler& clientHandler_;
    vector<Room> rooms;
    mutex roomsMutex;
};