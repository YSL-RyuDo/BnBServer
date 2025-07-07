#include "RoomManager.h"
#include "Server.h"

RoomManager::RoomManager(Server& server) : server_(server) {}

void RoomManager::BroadcastRoomlist(shared_ptr<ClientInfo> client)
{
    string message = "ROOM_LIST|";

    {
        lock_guard<mutex> lock(roomsMutex);
        for (size_t i = 0; i < rooms.size(); ++i) {
            const Room& room = rooms[i];
            message += room.roomName + "," + room.mapName + "," + (room.password.empty() ? "0" : "1");
            if (i != rooms.size() - 1)
                message += "|";  // 방 구분자
        }
    }

    message += "\n";

    int sendLen = send(client->socket, message.c_str(), (int)message.size(), 0);
    if (sendLen == SOCKET_ERROR) {
        std::cout << "[BroadcastRoomlist] 전송 실패: " << WSAGetLastError() << std::endl;
    }
    else {
        std::cout << "[BroadcastRoomlist] 전송 완료: " << message;
    }
}

bool RoomManager::CreateRoom(shared_ptr<ClientInfo> client, const string& roomName, const string& mapName, const string& password) {
    if (client->id.empty()) {
        std::cout << "[RoomManager::CreateRoom] ERROR: 로그인 안 된 클라이언트" << std::endl;
        string errorMsg = "ERROR|LOGIN_REQUIRED\n";
        send(client->socket, errorMsg.c_str(), (int)errorMsg.size(), 0);
        return false;
    }

    string userListStr;

    {
        lock_guard<mutex> lock(roomsMutex);

        Room newRoom;
        newRoom.roomName = roomName;
        newRoom.mapName = mapName;
        newRoom.password = password;
        newRoom.users.push_back(client->id); // 생성자 추가

        rooms.push_back(newRoom);
        std::cout << "[RoomManager] 방 생성 - 이름: " << roomName << ", 맵: " << mapName << ", by " << client->id << std::endl;

        // 유저 목록 문자열 구성
        Room& createdRoom = rooms.back();
        for (size_t i = 0; i < createdRoom.users.size(); ++i) {
            if (i > 0) userListStr += ",";
            userListStr += createdRoom.users[i];
        }
    }

    // 생성자에게 응답
    string successMsg = "CREATE_ROOM_SUCCESS|" + roomName + "|" + mapName + "|CREATOR|" + userListStr + "\n";
    send(client->socket, successMsg.c_str(), (int)successMsg.size(), 0);

    // 나머지에게 브로드캐스트
    string broadcastMsg = "ROOM_CREATED|" + roomName + "|" + mapName + "|" + userListStr + "\n";
    BroadcastMessageExcept(client->socket, broadcastMsg);

    return true;
}

void RoomManager::BroadcastMessageExcept(SOCKET exceptSocket, const string& message) {
    lock_guard<mutex> lock(server_.clientsMutex);
    string msgWithNewline = message;
    if (!msgWithNewline.empty() && msgWithNewline.back() != '\n') {
        msgWithNewline += "\n";
    }

    for (const auto& otherClient : server_.GetClients()) {
        if (otherClient->socket == exceptSocket) continue;
        send(otherClient->socket, msgWithNewline.c_str(), (int)msgWithNewline.size(), 0);
        cout << "[브로드캐스트 (제외)] → " << otherClient->ip << ":" << otherClient->port << " : " << msgWithNewline;
    }
}
