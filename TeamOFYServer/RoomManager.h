#pragma once
#include <vector>
#include <string>
#include <mutex>
#include <memory>
#include "Room.h"
#include "ClientInfo.h"
class Server;
using namespace std;

class RoomManager {
public:
    RoomManager(Server& server);

    // 방 목록 전체 전송
    void BroadcastRoomlist(shared_ptr<ClientInfo> client);
    bool CreateRoom(shared_ptr<ClientInfo> client, const string& roomName, const string& mapName, const string& password);
    void BroadcastMessageExcept(SOCKET exceptSocket, const string& message);
    // 방 추가 등 확장 가능
    /*void AddRoom(const Room& room);
    void RemoveRoom(const string& roomName);
    vector<Room> GetRooms();*/

private:
    Server& server_;
    vector<Room> rooms;
    mutex roomsMutex;
};