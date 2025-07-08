#include "RoomManager.h"
#include "Server.h"

RoomManager::RoomManager(Server& server, ClientHandler& clientHandler)
    : server_(server), clientHandler_(clientHandler) {}

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

// RoomManager Ŭ���� ��
bool RoomManager::CreateRoom(shared_ptr<ClientInfo> client, const string& roomName, const string& mapName, const string& password) {
    if (client->id.empty()) {
        cout << "[RoomManager] �α��� �� �� Ŭ���̾�Ʈ" << endl;
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
        newRoom.users.push_back(client->id);

        rooms.push_back(newRoom);

        Room& createdRoom = rooms.back();

        for (size_t i = 0; i < createdRoom.users.size(); ++i) {
            if (i > 0) userListStr += ",";
            userListStr += createdRoom.users[i];
        }
    }

    string successMsg = "CREATE_ROOM_SUCCESS|" + roomName + "|" + mapName + "|CREATOR|" + userListStr + "\n";
    send(client->socket, successMsg.c_str(), (int)successMsg.size(), 0);

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

bool RoomManager::EnterRoom(shared_ptr<ClientInfo> client, const string& roomName, const string& password, string& outResponse) {
    const int maxPlayers = 4;
    bool found = false;
    bool passwordMatch = false;
    string mapName;
    string userListStr;

    lock_guard<mutex> lock(roomsMutex);
    for (auto& room : rooms) {
        if (room.roomName == roomName) {
            found = true;
            if (room.password == password) {
                if (room.users.size() >= maxPlayers) {
                    outResponse = "ROOM_FULL\n";
                    return false;
                }
                passwordMatch = true;
                mapName = room.mapName;

                // �ߺ� ���� ����
                if (std::find(room.users.begin(), room.users.end(), client->id) == room.users.end()) {
                    room.users.push_back(client->id);
                }

                // ���� ����Ʈ ���ڿ� ����
                userListStr.clear();
                for (size_t i = 0; i < room.users.size(); ++i) {
                    if (i > 0) userListStr += ",";
                    userListStr += room.users[i];
                }

                // ���� ���� �޽���
                outResponse = "ENTER_ROOM_SUCCESS|" + roomName + "|" + userListStr + "\n";

                // �ٸ� �������� ���� �޽��� ��ε�ĳ��Ʈ
                string refreshMsg = "REFRESH_ROOM_SUCCESS|" + roomName + "|" + userListStr + "\n";
                auto& clientsMap = clientHandler_.GetClientsMap();

                for (const auto& user : room.users) {
                    if (user == client->id) continue;
                    auto it = clientsMap.find(user);
                    if (it != clientsMap.end()) {
                        send(it->second->socket, refreshMsg.c_str(), (int)refreshMsg.size(), 0);
                    }
                }

                return true;
            }
            break;
        }
    }

    if (!found) {
        outResponse = "ROOM_NOT_FOUND\n";
    }
    else if (!passwordMatch) {
        outResponse = "WRONG_ROOM_PASSWORD\n";
    }
    return false;
}

void RoomManager::HandleRoomChatMessage(shared_ptr<ClientInfo> sender, const string& data)
{
    size_t firstDelim = data.find(':');
    size_t secondDelim = data.find(':', firstDelim + 1);
    if (firstDelim == string::npos || secondDelim == string::npos)
    {
        cerr << "[RoomManager] �� �޽��� ���� ����: " << data << endl;
        return;
    }

    string roomName = data.substr(0, firstDelim);
    string nickname = data.substr(firstDelim + 1, secondDelim - firstDelim - 1);
    string chatMsg = data.substr(secondDelim + 1);

    cout << "[RoomChat] " << roomName << " - " << nickname << ": " << chatMsg << endl;

    string fullMsg = "ROOM_CHAT|" + roomName + "|" + nickname + ":" + chatMsg + "\n";

    lock_guard<mutex> lock(roomsMutex);

    for (const auto& room : rooms)
    {
        if (room.roomName == roomName)
        {
            auto& clientsMap = clientHandler_.GetClientsMap();

            for (const auto& user : room.users)
            {
                auto it = clientsMap.find(user);
                if (it != clientsMap.end())
                {
                    SOCKET otherSocket = it->second->socket;
                    send(otherSocket, fullMsg.c_str(), (int)fullMsg.size(), 0);
                }
            }
            break;
        }
    }
}

void RoomManager::ExitRoom(const std::string& message) {
    std::string data = message.substr(strlen("EXIT_ROOM|"));
    std::stringstream ss(data);
    std::string roomName, nickname;
    bool roomDeleted = false;
    getline(ss, roomName, '|');
    getline(ss, nickname);

    std::string userListStr;

    {
        std::lock_guard<std::mutex> lock(roomsMutex);
        auto it = std::find_if(rooms.begin(), rooms.end(), [&](const Room& r) {
            return r.roomName == roomName;
            });

        if (it != rooms.end()) {
            Room& room = *it;

            // ���� ����
            auto userIt = std::remove(room.users.begin(), room.users.end(), nickname);
            if (userIt != room.users.end()) {
                room.users.erase(userIt, room.users.end());
                std::cout << "[����] " << nickname << " ���� �� '" << roomName << "' ���� ����\n";

                // ���� �� ������ 0���̸� �� ����
                if (room.users.empty()) {
                    rooms.erase(it);
                    std::cout << "[����] �� '" << roomName << "' �� ������ ���� ������\n";
                    roomDeleted = true;
                }
                else {
                    // ���� ���� ����Ʈ ����
                    for (const auto& user : room.users) {
                        if (!userListStr.empty()) userListStr += ",";
                        userListStr += user;
                    }

                    // ���� �������� ���� �޽��� ����
                    std::string refreshMsg = "REFRESH_ROOM_SUCCESS|" + roomName + "|" + userListStr + "\n";

                    // Ŭ���̾�Ʈ ���ؽ� ���
                    std::lock_guard<std::mutex> lockClients(server_.clientsMutex);

                    for (const auto& user : room.users) {
                        auto clientIt = clientHandler_.GetClientsMap().find(user);
                        if (clientIt != clientHandler_.GetClientsMap().end()) {
                            SOCKET s = clientIt->second->socket;
                            int sendLen = send(s, refreshMsg.c_str(), (int)refreshMsg.size(), 0);
                            if (sendLen == SOCKET_ERROR) {
                                std::cout << "[�� ��ε�ĳ��Ʈ ����] " << WSAGetLastError() << " - user: " << user << std::endl;
                            }
                            else {
                                std::cout << "[�� ��ε�ĳ��Ʈ] " << user << " ���� ���۵�: " << refreshMsg;
                            }
                        }
                        else {
                            std::cout << "[Ŭ���̾�Ʈ �˻� ����] user: " << user << " �� clientsMap���� ã�� ����\n";
                        }
                    }
                }
            }
            else {
                std::cout << "[����] " << nickname << " ��(��) �� '" << roomName << "' �� �������� ����\n";
            }
        }
        else {
            std::cout << "[����] �� '" << roomName << "' �� ã�� �� ����\n";
        }
    }

    if (roomDeleted) {
        // ���� �����Ǿ����� ��� Ŭ���̾�Ʈ���� �ֽ� �� ����Ʈ ��ε�ĳ��Ʈ
        auto& clients = server_.GetClients();
        std::lock_guard<std::mutex> lock(server_.clientsMutex);
        for (const auto& c : clients) {
            SendRoomList(*c);
        }
    }

    //BroadcastLobbyUserList(); // �κ� ���� ����Ʈ ���� ��ε�ĳ��Ʈ
}

void RoomManager::SendRoomList(ClientInfo& client) {
    string message = "ROOM_LIST|";

    {
        lock_guard<mutex> lock(roomsMutex);
        for (size_t i = 0; i < rooms.size(); ++i) {
            const Room& room = rooms[i];
            message += room.roomName + "|" + room.mapName + "|" + (room.password.empty() ? "0" : "1");
            if (i != rooms.size() - 1)
                message += ",";
        }
    }

    message += "\n";
    send(client.socket, message.c_str(), (int)message.size(), 0);
    cout << "[SendRoomList] ���� �޽��� -> IP: " << client.ip
        << ", Port: " << client.port << " -> " << message;
}