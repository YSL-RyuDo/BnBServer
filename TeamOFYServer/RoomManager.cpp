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

// RoomManager 클래스 내
bool RoomManager::CreateRoom(shared_ptr<ClientInfo> client, const string& roomName, const string& mapName, const string& password) {
    if (client->id.empty()) {
        cout << "[RoomManager] 로그인 안 된 클라이언트" << endl;
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
        cout << "[브로드캐스트 (제외)] → " << otherClient->ip << ":" << otherClient->port << " : " << msgWithNewline;
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

                // 중복 입장 방지
                if (std::find(room.users.begin(), room.users.end(), client->id) == room.users.end()) {
                    room.users.push_back(client->id);
                }

                // 유저 리스트 문자열 생성
                userListStr.clear();
                for (size_t i = 0; i < room.users.size(); ++i) {
                    if (i > 0) userListStr += ",";
                    userListStr += room.users[i];
                }

                // 입장 성공 메시지
                outResponse = "ENTER_ROOM_SUCCESS|" + roomName + "|" + userListStr + "\n";

                // 다른 유저에게 갱신 메시지 브로드캐스트
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
        cerr << "[RoomManager] 방 메시지 포맷 오류: " << data << endl;
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

            // 유저 제거
            auto userIt = std::remove(room.users.begin(), room.users.end(), nickname);
            if (userIt != room.users.end()) {
                room.users.erase(userIt, room.users.end());
                std::cout << "[서버] " << nickname << " 님이 방 '" << roomName << "' 에서 퇴장\n";

                // 퇴장 후 유저가 0명이면 방 제거
                if (room.users.empty()) {
                    rooms.erase(it);
                    std::cout << "[서버] 방 '" << roomName << "' 에 유저가 없어 삭제됨\n";
                    roomDeleted = true;
                }
                else {
                    // 남은 유저 리스트 구성
                    for (const auto& user : room.users) {
                        if (!userListStr.empty()) userListStr += ",";
                        userListStr += user;
                    }

                    // 남은 유저에게 갱신 메시지 전송
                    std::string refreshMsg = "REFRESH_ROOM_SUCCESS|" + roomName + "|" + userListStr + "\n";

                    // 클라이언트 뮤텍스 잠금
                    std::lock_guard<std::mutex> lockClients(server_.clientsMutex);

                    for (const auto& user : room.users) {
                        auto clientIt = clientHandler_.GetClientsMap().find(user);
                        if (clientIt != clientHandler_.GetClientsMap().end()) {
                            SOCKET s = clientIt->second->socket;
                            int sendLen = send(s, refreshMsg.c_str(), (int)refreshMsg.size(), 0);
                            if (sendLen == SOCKET_ERROR) {
                                std::cout << "[방 브로드캐스트 실패] " << WSAGetLastError() << " - user: " << user << std::endl;
                            }
                            else {
                                std::cout << "[방 브로드캐스트] " << user << " 에게 전송됨: " << refreshMsg;
                            }
                        }
                        else {
                            std::cout << "[클라이언트 검색 실패] user: " << user << " 를 clientsMap에서 찾지 못함\n";
                        }
                    }
                }
            }
            else {
                std::cout << "[서버] " << nickname << " 은(는) 방 '" << roomName << "' 에 존재하지 않음\n";
            }
        }
        else {
            std::cout << "[서버] 방 '" << roomName << "' 을 찾을 수 없음\n";
        }
    }

    if (roomDeleted) {
        // 방이 삭제되었으면 모든 클라이언트에게 최신 방 리스트 브로드캐스트
        auto& clients = server_.GetClients();
        std::lock_guard<std::mutex> lock(server_.clientsMutex);
        for (const auto& c : clients) {
            SendRoomList(*c);
        }
    }

    //BroadcastLobbyUserList(); // 로비 유저 리스트 갱신 브로드캐스트
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
    cout << "[SendRoomList] 보낸 메시지 -> IP: " << client.ip
        << ", Port: " << client.port << " -> " << message;
}