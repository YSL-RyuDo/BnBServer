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
                message += "|";  // �� ������
        }
    }

    message += "\n";

    int sendLen = send(client->socket, message.c_str(), (int)message.size(), 0);
    if (sendLen == SOCKET_ERROR) {
        std::cout << "[BroadcastRoomlist] ���� ����: " << WSAGetLastError() << std::endl;
    }
    else {
        std::cout << "[BroadcastRoomlist] ���� �Ϸ�: " << message;
    }
}

bool RoomManager::CreateRoom(shared_ptr<ClientInfo> client, const string& roomName, const string& mapName, const string& password) {
    if (client->id.empty()) {
        std::cout << "[RoomManager::CreateRoom] ERROR: �α��� �� �� Ŭ���̾�Ʈ" << std::endl;
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
        newRoom.users.push_back(client->id); // ������ �߰�

        rooms.push_back(newRoom);
        std::cout << "[RoomManager] �� ���� - �̸�: " << roomName << ", ��: " << mapName << ", by " << client->id << std::endl;

        // ���� ��� ���ڿ� ����
        Room& createdRoom = rooms.back();
        for (size_t i = 0; i < createdRoom.users.size(); ++i) {
            if (i > 0) userListStr += ",";
            userListStr += createdRoom.users[i];
        }
    }

    // �����ڿ��� ����
    string successMsg = "CREATE_ROOM_SUCCESS|" + roomName + "|" + mapName + "|CREATOR|" + userListStr + "\n";
    send(client->socket, successMsg.c_str(), (int)successMsg.size(), 0);

    // ���������� ��ε�ĳ��Ʈ
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
        cout << "[��ε�ĳ��Ʈ (����)] �� " << otherClient->ip << ":" << otherClient->port << " : " << msgWithNewline;
    }
}
