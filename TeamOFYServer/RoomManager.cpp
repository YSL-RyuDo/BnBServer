#include "RoomManager.h"
#include "Server.h"

RoomManager::RoomManager(Server& server, ClientHandler& clientHandler)
    : server_(server), clientHandler_(clientHandler) {}

//void RoomManager::BroadcastRoomlist(shared_ptr<ClientInfo> client)
//{
//    string message = "ROOM_LIST|";
//
//    {
//        lock_guard<mutex> lock(roomsMutex);
//        for (size_t i = 0; i < rooms.size(); ++i) {
//            const Room& room = rooms[i];
//            message += room.roomName + "," + room.mapName + "," + (room.password.empty() ? "0" : "1");
//            if (i != rooms.size() - 1)
//                message += "|";  // �� ������
//        }
//    }
//
//    message += "\n";
//    cout << "[�븮��Ʈ]" << message << endl;
//    int sendLen = send(client->socket, message.c_str(), (int)message.size(), 0);
//    if (sendLen == SOCKET_ERROR) {
//        std::cout << "[BroadcastRoomlist] ���� ����: " << WSAGetLastError() << std::endl;
//    }
//    else {
//        std::cout << "[BroadcastRoomlist] ���� �Ϸ�: " << message;
//    }
//}

void RoomManager::BroadcastRoomlist(shared_ptr<ClientInfo> client)
{
    string message = "ROOM_LIST|";

    {
        lock_guard<mutex> lock(roomsMutex);
        for (size_t i = 0; i < rooms.size(); ++i) {
            const Room& room = rooms[i];
            message += room.roomName + "," + room.mapName + "," + (room.password.empty() ? "0" : "1") + "," + (room.isCoopMode ? "1" : "0");
            if (i != rooms.size() - 1)
                message += "|";  // �� ������
        }
    }

    message += "\n";
    cout << "[�븮��Ʈ]" << message << endl;
    int sendLen = send(client->socket, message.c_str(), (int)message.size(), 0);
    if (sendLen == SOCKET_ERROR) {
        std::cout << "[BroadcastRoomlist] ���� ����: " << WSAGetLastError() << std::endl;
    }
    else {
        std::cout << "[BroadcastRoomlist] ���� �Ϸ�: " << message;
    }
}


// RoomManager Ŭ���� ��
//bool RoomManager::CreateRoom(shared_ptr<ClientInfo> client, const string& roomName, const string& mapName, const string& password) {
//    if (client->id.empty()) {
//        cout << "[RoomManager] �α��� �� �� Ŭ���̾�Ʈ" << endl;
//        string errorMsg = "ERROR|LOGIN_REQUIRED\n";
//        send(client->socket, errorMsg.c_str(), (int)errorMsg.size(), 0);
//        return false;
//    }
//
//    string userListStr;
//
//    {
//        lock_guard<mutex> lock(roomsMutex);
//
//        Room newRoom;
//        newRoom.roomName = roomName;
//        newRoom.mapName = mapName;
//        newRoom.password = password;
//        newRoom.users.push_back(client->id);
//
//        rooms.push_back(newRoom);
//
//        Room& createdRoom = rooms.back();
//
//        for (size_t i = 0; i < createdRoom.users.size(); ++i) {
//            if (i > 0) userListStr += ",";
//            string username = createdRoom.users[i];
//            string rawNickname = clientHandler_.GetNicknameById(username);
//            string nickname = Trim(rawNickname);
//            int characterIndex = 0;
//
//            auto it = createdRoom.characterSelections.find(username);
//            if (it != createdRoom.characterSelections.end()) {
//                characterIndex = it->second;
//            }
//
//            userListStr += nickname + ":" + std::to_string(characterIndex);
//        }
//
//    }
//    string hasPasswordStr = password.empty() ? "NO_PASSWORD" : "HAS_PASSWORD";
//
//    string successMsg = "CREATE_ROOM_SUCCESS|" + roomName + "|" + mapName + "|CREATOR|" + userListStr + "|" + hasPasswordStr + "\n";
//    cout << successMsg << endl;
//    send(client->socket, successMsg.c_str(), (int)successMsg.size(), 0);
//
//    string broadcastMsg = "ROOM_CREATED|" + roomName + "|" + mapName + "|" + userListStr + "|" + hasPasswordStr + "\n";
//    BroadcastMessageExcept(client->socket, broadcastMsg);
//
//
//    return true;
//}
bool RoomManager::CreateRoom(
    shared_ptr<ClientInfo> client,
    const string& roomName,
    const string& mapName,
    const string& password,
    bool isCoopMode) // Ŭ���̾�Ʈ���� ���޵� ������ ����
{
    // �α��� Ȯ��
    if (client->id.empty()) {
        cout << "[RoomManager] �α��� �� �� Ŭ���̾�Ʈ" << endl;
        string errorMsg = "ERROR|LOGIN_REQUIRED\n";
        send(client->socket, errorMsg.c_str(), (int)errorMsg.size(), 0);
        return false;
    }

    string userListStr;

    {
        lock_guard<mutex> lock(roomsMutex);

        // �� �� ����
        Room newRoom;
        newRoom.roomName = roomName;
        newRoom.mapName = mapName;
        newRoom.password = password;
        newRoom.isCoopMode = isCoopMode; // �� ������ ���� ����
        newRoom.users.push_back(client->id);

        rooms.push_back(newRoom);
        Room& createdRoom = rooms.back();

        // ���� ��� ���ڿ� ����
        for (size_t i = 0; i < createdRoom.users.size(); ++i) {
            if (i > 0) userListStr += ",";
            string username = createdRoom.users[i];
            string rawNickname = clientHandler_.GetNicknameById(username);
            string nickname = Trim(rawNickname);
            int characterIndex = 0;

            auto it = createdRoom.characterSelections.find(username);
            if (it != createdRoom.characterSelections.end()) {
                characterIndex = it->second;
            }

            userListStr += nickname + ":" + std::to_string(characterIndex);
        }
    }

    // ��й�ȣ �� ������ ���� ���ڿ�
    string hasPasswordStr = password.empty() ? "NO_PASSWORD" : "HAS_PASSWORD";
    string coopStr = isCoopMode ? "1" : "0"; // 1 = ������, 0 = ������

    // Ŭ���̾�Ʈ���� ���� �޽��� ����
    string successMsg = "CREATE_ROOM_SUCCESS|" + roomName + "|" + mapName + "|CREATOR|"
        + userListStr + "|" + hasPasswordStr + "|" + coopStr + "\n";
    cout << successMsg << endl;
    send(client->socket, successMsg.c_str(), (int)successMsg.size(), 0);

    // �ٸ� Ŭ���̾�Ʈ�鿡�� �� ���� ��ε�ĳ��Ʈ
    string broadcastMsg = "ROOM_CREATED|" + roomName + "|" + mapName + "|"
        + userListStr + "|" + hasPasswordStr + "|" + coopStr + "\n";
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
        //cout << "[��ε�ĳ��Ʈ (����)] �� " << otherClient->ip << ":" << otherClient->port << " : " << msgWithNewline;
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
            cout << "[EnterRoom] �� ã��: " << roomName << endl;

            if (room.password == password) {
                cout << "[EnterRoom] ��й�ȣ ��ġ" << endl;

                if (room.isInGame) {
                    outResponse = "GAME_ALREADY_STARTED\n";
                    cout << "[Send to Client] " << outResponse;
                    return false;
                }

                if (room.users.size() >= maxPlayers) {
                    outResponse = "ROOM_FULL\n";
                    cout << "[Send to Client] " << outResponse;
                    return false;
                }

                passwordMatch = true;
                mapName = room.mapName;

                // ������ ���� �濡 ������ �߰�
                if (std::find(room.users.begin(), room.users.end(), client->id) == room.users.end()) {
                    room.users.push_back(client->id);
                    cout << "[EnterRoom] ����� " << client->id << " ���� ó�� �Ϸ�" << endl;

                    // ĳ���� ���� ������ ������ �⺻�� 0���� �ʱ�ȭ
                    if (room.characterSelections.find(client->id) == room.characterSelections.end()) {
                        room.characterSelections[client->id] = 0;
                    }
                }
                else {
                    cout << "[EnterRoom] ����� " << client->id << " �̹� ���� ����" << endl;
                }

                userListStr.clear();
                for (size_t i = 0; i < room.users.size(); ++i) {
                    if (i > 0) userListStr += ",";

                    const std::string& userId = room.users[i];
                    std::string rawNickname = clientHandler_.GetNicknameById(userId);
                    string nickname = Trim(rawNickname);

                    int charIndex = 0;
                    auto itChar = room.characterSelections.find(userId);
                    if (itChar != room.characterSelections.end()) {
                        charIndex = itChar->second;
                    }

                    userListStr += nickname + ":" + std::to_string(charIndex);
                }

                cout << "[EnterRoom] ���� ����Ʈ: " << userListStr << endl;

                outResponse = "ENTER_ROOM_SUCCESS|" + roomName + "|" + mapName + "|" + userListStr + "\n";
                cout << "[Send to Client] " << outResponse;

                // �濡 �ִ� �ٸ� �����鿡�� ���� �޽��� ����
                string refreshMsg = "REFRESH_ROOM_SUCCESS|" + roomName + "|" + mapName + "|" + userListStr + "\n";
                cout << "[Broadcast to Others] " << refreshMsg;

                auto& clientsMap = clientHandler_.GetClientsMap();
                for (const auto& user : room.users) {
                    //if (user == client->id) continue;

                    auto it = clientsMap.find(user);
                    if (it != clientsMap.end()) {
                        send(it->second->socket, refreshMsg.c_str(), (int)refreshMsg.size(), 0);
                        cout << "[EnterRoom] �ٸ� ���� " << user << "���� ���� �޽��� ����" << endl;
                    }
                }

                return true;
            }
            else {
                cout << "[EnterRoom] ��й�ȣ ����ġ" << endl;
            }

            break; // ���� ã�ұ� ������ ���� ����
        }
    }

    // ���� ���̽� ó��
    if (!found) {
        outResponse = "ROOM_NOT_FOUND\n";
        cout << "[Send to Client] " << outResponse;
    }
    else if (!passwordMatch) {
        outResponse = "WRONG_ROOM_PASSWORD\n";
        cout << "[Send to Client] " << outResponse;
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
    string rawuserId = clientHandler_.GetIdByNickname(nickname);
    string userId = Trim(rawuserId);
    {
        std::lock_guard<std::mutex> lock(roomsMutex);
        auto it = std::find_if(rooms.begin(), rooms.end(), [&](const Room& r) {
            return r.roomName == roomName;
            });

        if (it != rooms.end()) {
            Room& room = *it;

            // ���� ����
            auto userIt = std::remove(room.users.begin(), room.users.end(), userId);
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
                    for (const auto& userId : room.users) {
                        if (!userListStr.empty()) userListStr += ",";

                        string rawNickname = clientHandler_.GetNicknameById(userId);

                        string nickname = Trim(rawNickname);

                        int charIndex = 0;
                        auto charIt = room.characterSelections.find(userId);
                        if (charIt != room.characterSelections.end()) {
                            charIndex = charIt->second;
                        }

                        userListStr += nickname + ":" + std::to_string(charIndex);
                    }

                    // ���� �������� ���� �޽��� ����
                    std::string refreshMsg = "REFRESH_ROOM_SUCCESS|" + roomName + "|" + room.mapName + "|" + userListStr + "\n";

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
}

void RoomManager::SendRoomList(ClientInfo& client) {
    std::string message = "ROOM_LIST|"; 

    {
        std::lock_guard<std::mutex> lock(roomsMutex);
        bool first = true;

        for (const Room& room : rooms) {
            if (!first) {
                message += "|";  // �� �� ������ |�� ����
            }
            message += room.roomName + "," + room.mapName + "," + (room.password.empty() ? "0" : "1");
            first = false;
        }
    }

    message += "\n";  // �ٹٲ����� �޽��� ����
    std::cout << "[SendRoomList] ���� �޽��� -> IP: " << client.ip
        << ", Port: " << client.port << " -> " << message << endl;
    send(client.socket, message.c_str(), (int)message.size(), 0);

    
}

void RoomManager::HandleCharacterChoice(ClientInfo& client, const std::string& data)
{
    std::stringstream ss(data);
    std::string roomName, nickname, characterIndexStr;

    if (!std::getline(ss, roomName, '|') ||
        !std::getline(ss, nickname, '|') ||
        !std::getline(ss, characterIndexStr)) {
        std::cout << "[���] CHOOSE_CHARACTER �Ľ� ����: " << data << std::endl;
        return;
    }

    

    int characterIndex = 0;
    try {
        characterIndex = std::stoi(characterIndexStr);
    }
    catch (const std::exception& e) {
        std::cout << "[����] characterIndex �Ľ� ����: " << characterIndexStr << " (" << e.what() << ")" << std::endl;
        return;
    }

    string rawuserId = clientHandler_.GetIdByNickname(nickname);
    string userId = Trim(rawuserId);
    std::lock_guard<std::mutex> lock(roomsMutex);

    auto it = std::find_if(rooms.begin(), rooms.end(), [&](const Room& r) {
        return r.roomName == roomName;
        });

    if (it != rooms.end())
    {
        Room& room = *it;

        // �г����� ������ �����ϴ��� Ȯ��
        auto userIt = std::find(room.users.begin(), room.users.end(), userId);
        if (userIt == room.users.end()) {
            std::cout << "[���] " << userId << " ��(��) �� '" << roomName << "' �� �������� ����\n";
            return;
        }

        // ĳ���� ���� ���� ����
        room.characterSelections[userId] = characterIndex;

        std::string msg = "UPDATE_CHARACTER|" + nickname + "|" + std::to_string(characterIndex) + "\n";

        {
            std::lock_guard<std::mutex> lockClients(server_.clientsMutex);
            for (const auto& user : room.users) {
                auto clientIt = clientHandler_.GetClientsMap().find(user);
                if (clientIt != clientHandler_.GetClientsMap().end()) {
                    SOCKET s = clientIt->second->socket;
                    int sendLen = send(s, msg.c_str(), (int)msg.size(), 0);
                    if (sendLen == SOCKET_ERROR) {
                        std::cout << "[ĳ���� ��ε�ĳ��Ʈ ����] " << WSAGetLastError() << " - user: " << user << std::endl;
                    }
                    else {
                        std::cout << "[ĳ���� ��ε�ĳ��Ʈ] " << user << " ���� ���۵�: " << msg;
                    }
                }
                else {
                    std::cout << "[Ŭ���̾�Ʈ �˻� ����] user: " << user << " �� clientsMap���� ã�� ����\n";
                }
            }
        }
    }
    else {
        std::cout << "[���] �� '" << roomName << "' �� ã�� �� ����\n";
    }
}

bool RoomManager::TryStartGame(const string& roomName, vector<string>& usersOut) {
    lock_guard<mutex> lock(roomsMutex);
    for (auto& room : rooms) {
        if (room.roomName == roomName) {
            if (room.users.size() >= 2) {
                usersOut.clear();
                for (const auto& userId : room.users) {
                    std::string rawNickname = clientHandler_.GetNicknameById(userId);
                    string nickname = Trim(rawNickname);
                    usersOut.push_back(nickname);
                }
                room.isInGame = true;
                return true;
            }
            return false;
        }
    }
    return false;
}

Room* RoomManager::FindRoomByName(const std::string& roomName)
{
    std::lock_guard<std::mutex> lock(roomsMutex);
    for (auto& room : rooms)
    {
        if (room.roomName == roomName)
            return &room;
    }
    return nullptr;
}

string RoomManager::GetGameUserListResponse(const string& roomName)
{
    std::lock_guard<std::mutex> lock(roomsMutex);
    for (const auto& room : rooms)
    {
        if (room.roomName == roomName)
        {
            string response = "GAME_USER_LIST|";
            for (const string& nickname : room.users)
            {
                response += nickname + ",1;";
            }
            if (!room.users.empty()) response.pop_back();
            response += "\n";
            return response;
        }
    }
    return "GAME_USER_LIST|\n";
}

// RoomManager Ŭ���� ���ο� �߰�
void RoomManager::BroadcastToUserRoom(const std::string& senderId, const std::string& message) {
    std::lock_guard<std::mutex> lockRooms(roomsMutex);

    // senderId�� ���� �� ã��
    auto it = std::find_if(rooms.begin(), rooms.end(), [&](const Room& room) {
        return std::find(room.users.begin(), room.users.end(), senderId) != room.users.end();
        });

    if (it == rooms.end()) {
        std::cerr << "[BroadcastToUserRoom] senderId�� ���� ���� ã�� �� ����: " << senderId << std::endl;
        return;
    }

    Room& room = *it;

    std::lock_guard<std::mutex> lockClients(server_.clientsMutex);
    auto& clientsMap = clientHandler_.GetClientsMap();

    for (const auto& userId : room.users) {
        auto clientIt = clientsMap.find(userId);
        if (clientIt != clientsMap.end()) {
            int sendLen = send(clientIt->second->socket, message.c_str(), (int)message.size(), 0);
            if (sendLen == SOCKET_ERROR) {
                std::cerr << "[BroadcastToUserRoom] send ����: " << WSAGetLastError() << " - userId: " << userId << std::endl;
            }
        }
        else {
            std::cerr << "[BroadcastToUserRoom] Ŭ���̾�Ʈ �˻� ���� - userId: " << userId << std::endl;
        }
    }
}

void RoomManager::BroadcastToRoomExcept(SOCKET exceptSocket, const std::string& message)
{
    std::lock_guard<std::mutex> lockRooms(roomsMutex);

    // excludedSocket�� ���� �� ã��
    auto it = std::find_if(rooms.begin(), rooms.end(), [&](const Room& room) {
        for (const auto& userId : room.users)
        {
            auto clientIt = clientHandler_.GetClientsMap().find(userId);
            if (clientIt != clientHandler_.GetClientsMap().end())
            {
                if (clientIt->second->socket == exceptSocket)
                    return true;
            }
        }
        return false;
        });

    if (it == rooms.end()) return;

    Room& room = *it;

    std::lock_guard<std::mutex> lockClients(server_.clientsMutex);
    auto& clientsMap = clientHandler_.GetClientsMap();

    std::string msgWithNewline = message;
    if (!msgWithNewline.empty() && msgWithNewline.back() != '\n') {
        msgWithNewline += "\n";
    }

    for (const auto& userId : room.users)
    {
        auto clientIt = clientsMap.find(userId);
        if (clientIt != clientsMap.end())
        {
            if (clientIt->second->socket == exceptSocket) continue;
            send(clientIt->second->socket, msgWithNewline.c_str(), (int)msgWithNewline.size(), 0);
        }
    }
}

std::string RoomManager::GetUserRoomId(const std::string& userId) {
    std::lock_guard<std::mutex> lockRooms(roomsMutex);

    auto it = std::find_if(rooms.begin(), rooms.end(), [&](const Room& room) {
        return std::find(room.users.begin(), room.users.end(), userId) != room.users.end();
        });

    if (it == rooms.end()) {
        std::cerr << "[GetUserRoomId] userId�� ���� ���� ã�� �� ����: " << userId << std::endl;
        return "";
    }

    return it->roomName;  // �� ����ü�� �� ID�� �ִٸ� �̷��� ����
}

// RoomManager.cpp
std::vector<std::string> RoomManager::GetUserIdsInRoom(const std::string& roomId)
{
    std::lock_guard<std::mutex> lock(roomsMutex);
    for (const auto& room : rooms)
    {
        if (room.roomName == roomId)
        {
            return room.users;
        }
    }
    return {};
}

bool RoomManager::HandleReadyToExit(const std::string& userId, const std::string& roomId, SOCKET excludeSocket)
{
    std::string nickname = Trim(clientHandler_.GetNicknameById(userId));
    bool allReady = false;
    std::vector<std::string> roomUsers;

    {
        std::lock_guard<std::mutex> lock(roomsMutex);
        readyUsersByRoom[roomId].insert(userId);

        auto it = std::find_if(rooms.begin(), rooms.end(),
            [&](const Room& r) { return r.roomName == roomId; });

        if (it == rooms.end())
            return false;

        Room& room = *it;
        roomUsers = room.users;

        allReady = true;
        for (const auto& user : roomUsers)
        {
            if (readyUsersByRoom[roomId].find(user) == readyUsersByRoom[roomId].end())
            {
                allReady = false;
                break;
            }
        }
        if (allReady)
        {
            // ���� ���� ���·� ����
            room.isInGame = false;
            std::cout << "[RoomManager] �� '" << room.roomName << "' isInGame = false �� ����� (���� ����)\n";
        }
    }

    // �غ� ��ȣ ��ε�ĳ��Ʈ
    {
        std::lock_guard<std::mutex> clientLock(server_.clientsMutex);
        auto& clientsMap = clientHandler_.GetClientsMap();

        for (const auto& uid : roomUsers)
        {
            if (uid == userId) continue; // �ڽ� ����

            auto it = clientsMap.find(uid);
            if (it != clientsMap.end())
            {
                SOCKET s = it->second->socket;
                std::string msg = "READY_TO_EXIT|" + nickname + "\n";
                send(s, msg.c_str(), (int)msg.size(), 0);
            }
        }
    }

    if (allReady)
    {
        // GAME_END ����
        std::string gameEndMsg = "GAME_END\n";

        std::lock_guard<std::mutex> clientLock(server_.clientsMutex);
        auto& clientsMap = clientHandler_.GetClientsMap();

        for (const auto& userId : roomUsers)
        {
            auto clientIt = clientsMap.find(userId);
            if (clientIt != clientsMap.end())
            {
                send(clientIt->second->socket, gameEndMsg.c_str(), (int)gameEndMsg.size(), 0);
            }
        }

        // �غ� ���� �ʱ�ȭ
        {
            std::lock_guard<std::mutex> lock(roomsMutex);

            std::cout << "[DEBUG] HandleReadyToExit: �ʱ�ȭ �� �غ� ���� ��: "
                << readyUsersByRoom[roomId].size() << ", ���� ���� ��: "
                << deadUsersByRoom[roomId].size() << std::endl;

            readyUsersByRoom[roomId].clear();
            deadUsersByRoom[roomId].clear();

            std::cout << "[DEBUG] HandleReadyToExit: �ʱ�ȭ �Ϸ�. �غ� ���� ��: "
                << readyUsersByRoom[roomId].size() << ", ���� ���� ��: "
                << deadUsersByRoom[roomId].size() << std::endl;
        }

    }

    return allReady;
}
