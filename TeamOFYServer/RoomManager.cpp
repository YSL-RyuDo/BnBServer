#include "RoomManager.h"
#include <random>
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

            string userListStr;
            for (size_t j = 0; j < room.users.size(); ++j) {
                if (j > 0) userListStr += ",";
                string username = room.users[j];
                string rawNickname = clientHandler_.GetNicknameById(username);
                string nickname = Trim(rawNickname);
                int characterIndex = 0;
                auto it = room.characterSelections.find(username);
                if (it != room.characterSelections.end()) characterIndex = it->second;

                string team = "None";
                if (room.isCoopMode) {
                    auto teamIt = room.teamAssignments.find(username);
                    if (teamIt != room.teamAssignments.end()) team = teamIt->second;
                }

                userListStr += nickname + ":" + std::to_string(characterIndex) + ":" + team;
            }

            message += room.roomName + "," + room.mapName + "," + (room.password.empty() ? "0" : "1") + "," + (room.isCoopMode ? "1" : "0") + "," + userListStr;

            if (i != rooms.size() - 1) message += "|";
        }
    }

    message += "\n";
    cout << "[룸리스트]" << message << endl;
    int sendLen = QueueSend(client->socket, message.c_str(), (int)message.size(), 0);
    if (sendLen == SOCKET_ERROR) {
        std::cout << "[BroadcastRoomlist] 전송 실패: " << WSAGetLastError() << std::endl;
    }
    else {
        std::cout << "[BroadcastRoomlist] 전송 완료: " << message;
    }
}

bool RoomManager::CreateRoom(
    shared_ptr<ClientInfo> client,
    const string& roomName,
    const string& mapName,
    const string& password,
    bool isCoopMode) // 클라이언트에서 전달된 협동전 여부
{
    // 로그인 확인
    if (client->id.empty()) {
        cout << "[RoomManager] 로그인 안 된 클라이언트" << endl;
        string errorMsg = "ERROR|LOGIN_REQUIRED\n";
        QueueSend(client->socket, errorMsg.c_str(), (int)errorMsg.size(), 0);
        return false;
    }

    string userListStr;

    {
        lock_guard<mutex> lock(roomsMutex);
        for (const auto& room : rooms) {
            if (room.roomName == roomName || std::find(room.users.begin(), room.users.end(), client->id) != room.users.end()) return false;
        }

        // 새 룸 생성
        Room newRoom;
        newRoom.roomName = roomName;
        newRoom.mapName = mapName;
        newRoom.password = password;
        newRoom.isCoopMode = isCoopMode; // ← 협동전 여부 설정
        newRoom.users.push_back(client->id);

        if (isCoopMode) {
            newRoom.teamAssignments[client->id] = "Blue";
        }

        rooms.push_back(newRoom);
        Room& createdRoom = rooms.back();

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

            string team = "None";
            if (createdRoom.isCoopMode) {
                auto teamIt = createdRoom.teamAssignments.find(username);
                if (teamIt != createdRoom.teamAssignments.end())
                    team = teamIt->second;
            }

            userListStr += nickname + ":" + std::to_string(characterIndex) + ":" + team;
        }

    }

    // 비밀번호 및 협동전 여부 문자열
    string hasPasswordStr = password.empty() ? "NO_PASSWORD" : "HAS_PASSWORD";
    string coopStr = isCoopMode ? "1" : "0"; // 1 = 협동전, 0 = 개인전

    // 클라이언트에게 성공 메시지 전송
    string successMsg = "CREATE_ROOM_SUCCESS|" + roomName + "|" + mapName + "|CREATOR|"
        + userListStr + "|" + hasPasswordStr + "|" + coopStr + "\n";
    cout << successMsg << endl;
    QueueSend(client->socket, successMsg.c_str(), (int)successMsg.size(), 0);

    // 다른 클라이언트들에게 방 생성 브로드캐스트
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
        QueueSend(otherClient->socket, msgWithNewline.c_str(), (int)msgWithNewline.size(), 0);
        //cout << "[브로드캐스트 (제외)] → " << otherClient->ip << ":" << otherClient->port << " : " << msgWithNewline;
    }
}
bool RoomManager::EnterRoom(
    shared_ptr<ClientInfo> client,
    const string& roomName,
    const string& password,
    bool isCoopMode,
    string& response)
{
    if (client->id.empty()) {
        response = "ERROR|LOGIN_REQUIRED\n";
        return false;
    }

    lock_guard<mutex> lock(roomsMutex);
    for (const auto& room : rooms) if (std::find(room.users.begin(), room.users.end(), client->id) != room.users.end()) {
        response = "ENTER_ROOM_FAIL|ALREADY_IN_ROOM\n"; return false;
    }

    auto it = std::find_if(rooms.begin(), rooms.end(),
        [&roomName](const Room& r) { return r.roomName == roomName; });

    if (it == rooms.end()) {
        response = "ENTER_ROOM_FAILED|ROOM_NOT_FOUND\n";
        return false;
    }

    Room& room = *it;

    if (!room.password.empty() && room.password != password) {
        response = "ENTER_ROOM_FAILED|WRONG_PASSWORD\n";
        return false;
    }

    if (room.isInGame) {
        response = "ENTER_ROOM_FAILED|ALREADY_IN_GAME\n";
        return false;
    }

    // 유저 추가
    if (room.users.size() >= 4) { response = "ENTER_ROOM_FAIL|ROOM_FULL\n"; return false; }
            room.users.push_back(client->id);

    // 협동전이면 팀 할당
    if (room.isCoopMode) {
        size_t userIndex = room.users.size() - 1; // 새로 들어온 유저 인덱스
        string team = (userIndex % 2 == 0) ? "Blue" : "Red";
        room.teamAssignments[client->id] = team;
    }

    // 유저 리스트 문자열 생성
    string userListStr;
    for (size_t i = 0; i < room.users.size(); ++i) {
        if (i > 0) userListStr += ",";
        string username = room.users[i];
        string rawNickname = clientHandler_.GetNicknameById(username);
        string nickname = Trim(rawNickname);
        int characterIndex = 0;

        auto charIt = room.characterSelections.find(username);
        if (charIt != room.characterSelections.end()) characterIndex = charIt->second;

        string team = "None";
        if (room.isCoopMode) {
            auto teamIt = room.teamAssignments.find(username);
            if (teamIt != room.teamAssignments.end()) team = teamIt->second;
        }

        userListStr += nickname + ":" + std::to_string(characterIndex) + ":" + team;
    }

    string hasPasswordStr = room.password.empty() ? "NO_PASSWORD" : "HAS_PASSWORD";
    string coopStr = room.isCoopMode ? "1" : "0";

    response = "ENTER_ROOM_SUCCESS|" + roomName + "|" + room.mapName + "|" + userListStr + "|" + hasPasswordStr + "|" + coopStr + "\n";

    // 다른 클라이언트에게 갱신 메시지 전송
    string refreshMsg = "REFRESH_ROOM_SUCCESS|" + roomName + "|" + room.mapName + "|" + userListStr + "\n";
    cout << "[Broadcast to Others] " << refreshMsg;
    BroadcastMessageExcept(client->socket, refreshMsg);

    return true;
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
            std::lock_guard<std::mutex> clientsLock(server_.clientsMutex);
            auto& clientsMap = clientHandler_.GetClientsMapLocked();

            for (const auto& user : room.users)
            {
                auto it = clientsMap.find(user);
                if (it != clientsMap.end())
                {
                    SOCKET otherSocket = it->second->socket;
                    QueueSend(otherSocket, fullMsg.c_str(), (int)fullMsg.size(), 0);
                }
            }
            break;
        }
    }
}

void RoomManager::SendServerMessageToRoom(const std::string& roomName, const std::string& chatMsg)
{
    std::string fullMsg = "ROOM_CHAT|" + roomName + "|Server:" + chatMsg + "\n";

    std::lock_guard<std::mutex> lock(roomsMutex);

    Room* room = FindRoomByName_NoLock(roomName);
    if (!room) return;

    std::lock_guard<std::mutex> clientsLock(server_.clientsMutex);
    auto& clientsMap = clientHandler_.GetClientsMapLocked();
    for (const auto& userId : room->users)
    {
        auto it = clientsMap.find(userId);
        if (it != clientsMap.end())
        {
            QueueSend(it->second->socket, fullMsg.c_str(), (int)fullMsg.size(), 0);
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

            // 유저 제거
            auto userIt = std::remove(room.users.begin(), room.users.end(), userId);
            if (userIt != room.users.end()) {
                room.users.erase(userIt, room.users.end());
                std::cout << "[서버] " << nickname << " 님이 방 '" << roomName << "' 에서 퇴장\n";

                // 퇴장 후 유저가 0명이면 방 제거
                if (room.users.empty()) {
                    deadUsersByRoom.erase(it->roomName);
            readyUsersByRoom.erase(it->roomName);
            resolvedRooms_.erase(it->roomName);
            rooms.erase(it);
                    std::cout << "[서버] 방 '" << roomName << "' 에 유저가 없어 삭제됨\n";
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

                    // 남은 유저에게 갱신 메시지 전송
                    std::string refreshMsg = "REFRESH_ROOM_SUCCESS|" + roomName + "|" + room.mapName + "|" + userListStr + "\n";

                    // 클라이언트 뮤텍스 잠금
                    std::lock_guard<std::mutex> lockClients(server_.clientsMutex);

                    for (const auto& user : room.users) {
                        auto clientIt = clientHandler_.GetClientsMapLocked().find(user);
                        if (clientIt != clientHandler_.GetClientsMapLocked().end()) {
                            SOCKET s = clientIt->second->socket;
                            int sendLen = QueueSend(s, refreshMsg.c_str(), (int)refreshMsg.size(), 0);
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
        std::vector<std::shared_ptr<ClientInfo>> clients;
        { std::lock_guard<std::mutex> lock(server_.clientsMutex); clients = server_.GetClients(); }
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
                message += "|";  // 각 방 정보는 |로 구분
            }
            message += room.roomName + "," + room.mapName + "," + (room.password.empty() ? "0" : "1");
            first = false;
        }
    }

    message += "\n";  // 줄바꿈으로 메시지 종료
    std::cout << "[SendRoomList] 보낸 메시지 -> IP: " << client.ip
        << ", Port: " << client.port << " -> " << message << endl;
    QueueSend(client.socket, message.c_str(), (int)message.size(), 0);

    
}

void RoomManager::HandleCharacterChoice(ClientInfo& client, const std::string& data)
{
    std::stringstream ss(data);
    std::string roomName, nickname, characterIndexStr;

    if (!std::getline(ss, roomName, '|') ||
        !std::getline(ss, nickname, '|') ||
        !std::getline(ss, characterIndexStr)) {
        std::cout << "[경고] CHOOSE_CHARACTER 파싱 실패: " << data << std::endl;
        return;
    }

    

    int characterIndex = 0;
    try {
        characterIndex = std::stoi(characterIndexStr);
    }
    catch (const std::exception& e) {
        std::cout << "[에러] characterIndex 파싱 실패: " << characterIndexStr << " (" << e.what() << ")" << std::endl;
        return;
    }

    if (characterIndexStr != std::to_string(characterIndex) || characterIndex < 0 || characterIndex > 6) return;
    if (nickname != client.nickname) return;
    string rawuserId = client.id;
    string userId = Trim(rawuserId);
    std::lock_guard<std::mutex> lock(roomsMutex);

    auto it = std::find_if(rooms.begin(), rooms.end(), [&](const Room& r) {
        return r.roomName == roomName;
        });

    if (it != rooms.end())
    {
        Room& room = *it;

        // 닉네임이 실제로 존재하는지 확인
        auto userIt = std::find(room.users.begin(), room.users.end(), userId);
        if (userIt == room.users.end()) {
            std::cout << "[경고] " << userId << " 은(는) 방 '" << roomName << "' 에 존재하지 않음\n";
            return;
        }

        // 캐릭터 선택 정보 저장
        if (room.isInGame) return;
        room.characterSelections[userId] = characterIndex;

        std::string msg = "UPDATE_CHARACTER|" + nickname + "|" + std::to_string(characterIndex) + "\n";

        {
            std::lock_guard<std::mutex> lockClients(server_.clientsMutex);
            for (const auto& user : room.users) {
                auto clientIt = clientHandler_.GetClientsMapLocked().find(user);
                if (clientIt != clientHandler_.GetClientsMapLocked().end()) {
                    SOCKET s = clientIt->second->socket;
                    int sendLen = QueueSend(s, msg.c_str(), (int)msg.size(), 0);
                    if (sendLen == SOCKET_ERROR) {
                        std::cout << "[캐릭터 브로드캐스트 실패] " << WSAGetLastError() << " - user: " << user << std::endl;
                    }
                    else {
                        std::cout << "[캐릭터 브로드캐스트] " << user << " 에게 전송됨: " << msg;
                    }
                }
                else {
                    std::cout << "[클라이언트 검색 실패] user: " << user << " 를 clientsMap에서 찾지 못함\n";
                }
            }
        }
    }
    else {
        std::cout << "[경고] 방 '" << roomName << "' 을 찾을 수 없음\n";
    }
}

bool RoomManager::TryStartGame(const string& roomName, vector<string>& usersOut) {
    lock_guard<mutex> lock(roomsMutex);
    for (auto& room : rooms) {
        if (room.roomName == roomName) {
            if (!room.isInGame && room.users.size() >= 2) {
                usersOut.clear();
                for (const auto& userId : room.users) {
                    std::string rawNickname = clientHandler_.GetNicknameById(userId);
                    string nickname = Trim(rawNickname);
                    usersOut.push_back(nickname);
                }
                if (!InitializeCombat(room)) return false;
                room.isInGame = true; room.matchId = ++nextMatchId_;
                deadUsersByRoom.erase(roomName); readyUsersByRoom.erase(roomName); resolvedRooms_.erase(roomName);
                return true;
            }
            return false;
        }
    }
    return false;
}

bool RoomManager::TryStartCoopGame(
    const string& roomName,
    vector<string>& userList,
    vector<string>& blueTeam,
    vector<string>& redTeam)
{
    lock_guard<mutex> lock(roomsMutex);

    for (auto& room : rooms)
    {
        if (room.roomName == roomName)
        {
            // 인원 4명 아니면 실패
            if (room.isInGame || room.users.size() != 4)
                return false;

            userList.clear();
            blueTeam.clear();
            redTeam.clear();

            for (const auto& userId : room.users)
            {
                string rawNickname = clientHandler_.GetNicknameById(userId);
                string nickname = Trim(rawNickname);

                string team = "None";
                auto teamIt = room.teamAssignments.find(userId);
                if (teamIt != room.teamAssignments.end())
                    team = teamIt->second;

                userList.push_back(nickname);

                if (team == "Blue" || team == "BLUE")
                    blueTeam.push_back(nickname);
                else if (team == "Red" || team == "RED")
                    redTeam.push_back(nickname);
            }

            // 팀 인원 조건 확인
            if (blueTeam.size() == 2 && redTeam.size() == 2)
            {
                if (!InitializeCombat(room)) return false;
                room.isInGame = true; room.matchId = ++nextMatchId_;
                deadUsersByRoom.erase(roomName); readyUsersByRoom.erase(roomName); resolvedRooms_.erase(roomName);
                return true;
            }

            return false; // 팀 조건 불만족
        }
    }
    return false; // 방 없음
}



std::shared_ptr<const Room> RoomManager::FindRoomByName(const std::string& roomName)
{
    std::lock_guard<std::mutex> lock(roomsMutex);
    cout << "[DEBUG] START_GAME 요청: '" << roomName << "'" << endl;
    for (auto& room : rooms)
    {
        cout << "[DEBUG] 방 리스트: '" << room.roomName << "'" << endl;
        if (room.roomName == roomName)
            return std::make_shared<const Room>(room);
    }
    return nullptr;
}

Room* RoomManager::FindRoomByName_NoLock(const std::string& roomName)
{
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

// RoomManager 클래스 내부에 추가
void RoomManager::BroadcastToUserRoom(const std::string& senderId, const std::string& message) {
    std::vector<std::shared_ptr<ClientInfo>> recipients;
    {
        std::lock_guard<std::mutex> roomLock(roomsMutex);
        auto room = std::find_if(rooms.begin(), rooms.end(), [&](const Room& r) {
            return std::find(r.users.begin(), r.users.end(), senderId) != r.users.end();
        });
        if (room == rooms.end()) return;
        std::lock_guard<std::mutex> clientLock(server_.clientsMutex);
        const auto& clients = clientHandler_.GetClientsMapLocked();
        for (const auto& id : room->users) {
            auto it = clients.find(id);
            if (it != clients.end()) recipients.push_back(it->second);
        }
    }
    // Retain connection ownership, then enqueue outside both shared state locks.
    for (const auto& client : recipients)
        QueueSend(client->socket, message.data(), static_cast<int>(message.size()), 0);
}

void RoomManager::BroadcastToRoomExcept(const std::string& senderId, SOCKET exceptSocket, const std::string& message) {
    std::vector<std::shared_ptr<ClientInfo>> recipients;
    {
        std::lock_guard<std::mutex> roomLock(roomsMutex);
        auto room = std::find_if(rooms.begin(), rooms.end(), [&](const Room& r) {
            return std::find(r.users.begin(), r.users.end(), senderId) != r.users.end();
        });
        if (room == rooms.end()) return;
        std::lock_guard<std::mutex> clientLock(server_.clientsMutex);
        const auto& clients = clientHandler_.GetClientsMapLocked();
        for (const auto& id : room->users) {
            auto it = clients.find(id);
            if (it != clients.end() && it->second->socket != exceptSocket) recipients.push_back(it->second);
        }
    }
    std::string framed = message;
    if (!framed.empty() && framed.back() != '\n') framed += '\n';
    for (const auto& client : recipients)
        QueueSend(client->socket, framed.data(), static_cast<int>(framed.size()), 0);
}

std::string RoomManager::GetUserRoomId(const std::string& userId) {
    std::lock_guard<std::mutex> lockRooms(roomsMutex);

    auto it = std::find_if(rooms.begin(), rooms.end(), [&](const Room& room) {
        return std::find(room.users.begin(), room.users.end(), userId) != room.users.end();
        });

    if (it == rooms.end()) {
        std::cerr << "[GetUserRoomId] userId가 속한 방을 찾을 수 없음: " << userId << std::endl;
        return "";
    }

    return it->roomName;  // 방 구조체에 방 ID가 있다면 이렇게 리턴
}

// RoomManager.cpp
std::shared_ptr<const Room> RoomManager::GetRoomByUserId(const std::string& userId) {
    std::lock_guard<std::mutex> lock(roomsMutex);

    auto it = std::find_if(rooms.begin(), rooms.end(), [&](Room& room) {
        return std::find(room.users.begin(), room.users.end(), userId) != room.users.end();
        });

    if (it == rooms.end()) {
        std::cerr << "[GetRoomByUserId] userId가 속한 방을 찾을 수 없음: " << userId << std::endl;
        return nullptr;
    }

    return std::make_shared<const Room>(*it); // Room* 반환
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

bool RoomManager::HandleReadyToExit(const std::string& userId, const std::string& roomId, SOCKET excludeSocket) {
    std::vector<std::string> members;
    bool allReady = false;
    {
        std::lock_guard<std::mutex> lock(roomsMutex);
        Room* room = FindRoomByName_NoLock(roomId);
        if (!room || !room->isInGame || !room->rewardsCommitted || std::find(room->users.begin(), room->users.end(), userId) == room->users.end()) return false;
        auto& ready = readyUsersByRoom[roomId];
        if (!ready.insert(userId).second) return false;
        members = room->users;
        allReady = std::all_of(members.begin(), members.end(), [&](const std::string& id) { return ready.count(id) != 0; });
        if (allReady) {
            room->isInGame = false;
            readyUsersByRoom.erase(roomId); deadUsersByRoom.erase(roomId); resolvedRooms_.erase(roomId);
        }
    }
    // No room lock is acquired while the client lock is held.
    const auto nickname = Trim(clientHandler_.GetNicknameById(userId));
    std::lock_guard<std::mutex> clientsLock(server_.clientsMutex);
    const auto& clients = clientHandler_.GetClientsMapLocked();
    for (const auto& id : members) {
        const auto it = clients.find(id);
        if (it == clients.end()) continue;
        if (id != userId) {
            const auto message = "READY_TO_EXIT|" + nickname + "\n";
            QueueSend(it->second->socket, message.data(), static_cast<int>(message.size()), 0);
        }
        if (allReady) QueueSend(it->second->socket, "GAME_END\n", 9, 0);
    }
    return allReady;
}

bool RoomManager::InitializeCombat(Room& room) {
    std::unordered_map<std::string, Room::Combatant> players;
    for (const auto& id : room.users) {
        auto selection = room.characterSelections.find(id);
        int character = selection == room.characterSelections.end() ? 0 : selection->second;
        Room::Combatant p;
        p.character = character;
        p.health = clientHandler_.CombatHealth(character);
        p.attack = clientHandler_.CombatAttack(character);
        if (p.health <= 0 || p.attack <= 0) return false;
        players.emplace(id, p);
    }
    auto map = clientHandler_.CombatMap(room.mapName);
    CombatWorld world;
    if (map.second.size() < room.users.size() || !world.Initialize(room.mapName, map.first, nextMatchId_ + 1)) return false;
    std::mt19937 random(std::random_device{}());
    std::shuffle(map.second.begin(), map.second.end(), random);
    room.assignedSpawnPositions.clear();
    for (size_t i = 0; i < room.users.size(); ++i) {
        const auto& id = room.users[i];
        const auto spawn = map.second[i];
        if (spawn.second < 13 || !world.AddPlayer(id, Trim(clientHandler_.GetNicknameById(id)), players.at(id).character,
                clientHandler_.CombatBalloon(id), {double(spawn.first), double(spawn.second - 13)})) return false;
        room.assignedSpawnPositions.push_back(spawn);
    }
    room.world = std::move(world);
    room.combat = std::move(players);
    room.combatRevision = 0;
    room.rewardsPending = room.rewardsCommitted = false;
    return true;
}

RoomManager::DamageResult RoomManager::ApplyDamage(const std::string&, const std::string&, int, uint64_t, bool) {
    // Old client collision reports cannot authorize damage.
    return {};
}

RoomManager::DamageResult RoomManager::ApplyDamageLocked(Room& room, const std::string& attacker,
    const std::string& target, bool water) {
        if (!room.isInGame || room.rewardsPending || resolvedRooms_.count(room.roomName)) return {};
        auto a = room.combat.find(attacker), t = room.combat.find(target);
        if (a == room.combat.end() || t == room.combat.end() || a->second.health <= 0 || t->second.health <= 0) return {};
        if (water) { if (attacker != target) return {}; }
        else {
            if (attacker == target) return {};
            if (room.isCoopMode) {
                auto at = room.teamAssignments.find(attacker), tt = room.teamAssignments.find(target);
                if (at == room.teamAssignments.end() || tt == room.teamAssignments.end() || at->second == tt->second) return {};
            }
        }
        auto now = std::chrono::steady_clock::now();
        if (now < t->second.nextDamage) return {};
        // Preserve the existing one-second per-victim invulnerability rule on the server.
        t->second.nextDamage = now + std::chrono::seconds(1);
        int damage = water ? 10 : a->second.attack;
        t->second.health = (std::max)(0, t->second.health - damage);
        DamageResult result;
        result.accepted = true; result.damage = damage; result.health = t->second.health;
        result.matchId = room.matchId; result.revision = ++room.combatRevision;
        if (t->second.health == 0) result.death = RecordDeathLocked(room, target);
        if (result.health == 0) room.world.players.at(target).alive = false;
        return result;
}

RoomManager::DeathResult RoomManager::RecordDeathLocked(Room& room, const std::string& userId) {
    auto player = room.combat.find(userId);
    if (!room.isInGame || resolvedRooms_.count(room.roomName) || player == room.combat.end() || player->second.health != 0) return {};
    auto& dead = deadUsersByRoom[room.roomName];
    if (!dead.insert(userId).second) return {};
    size_t survivors = 0; std::unordered_set<std::string> teams;
    for (const auto& id : room.users) if (!dead.count(id)) {
        ++survivors;
        auto team = room.teamAssignments.find(id);
        teams.insert(team == room.teamAssignments.end() ? "None" : team->second);
    }
    bool resolved = room.isCoopMode ? teams.size() == 1 : survivors == 1;
    if (resolved) { resolvedRooms_.insert(room.roomName); room.rewardsPending = true; }
    return {std::make_shared<const Room>(room), dead, resolved};
}

RoomManager::DeathResult RoomManager::RecordDeath(const std::string& userId) {
    std::lock_guard<std::mutex> lock(roomsMutex);
    for (auto& room : rooms)
        if (std::find(room.users.begin(), room.users.end(), userId) != room.users.end())
            return RecordDeathLocked(room, userId);
    return {};
}

void RoomManager::MarkRewardsCommitted(const Room& snapshot) {
    std::lock_guard<std::mutex> lock(roomsMutex);
    Room* room = FindRoomByName_NoLock(snapshot.roomName);
    if (room && room->matchId == snapshot.matchId) room->rewardsCommitted = true;
}

std::shared_ptr<const Room> RoomManager::ChangeTeam(const std::string& userId) {
    std::lock_guard<std::mutex> lock(roomsMutex);
    for (auto& room : rooms) {
        if (!room.isCoopMode || room.isInGame || std::find(room.users.begin(), room.users.end(), userId) == room.users.end()) continue;
        auto it = room.teamAssignments.find(userId);
        if (it == room.teamAssignments.end() || (it->second != "Blue" && it->second != "Red")) return nullptr;
        it->second = it->second == "Blue" ? "Red" : "Blue";
        return std::make_shared<const Room>(room);
    }
    return nullptr;
}

void RoomManager::ForceExitRoomByUserId(const std::string& userId)
{
    std::string roomName;
    bool roomDeleted = false;
    std::string userListStr;

    {
        std::lock_guard<std::mutex> lock(roomsMutex);

        auto it = std::find_if(rooms.begin(), rooms.end(),
            [&](const Room& r) {
                return std::find(r.users.begin(), r.users.end(), userId) != r.users.end();
            });

        if (it == rooms.end())
            return; // 방에 없음

        Room& room = *it;
        roomName = room.roomName;

        // 유저 제거
        room.users.erase(
            std::remove(room.users.begin(), room.users.end(), userId),
            room.users.end()
        );

        // 부가 정보 정리
        room.world.RemovePlayer(userId);
        room.characterSelections.erase(userId);
        room.teamAssignments.erase(userId);

        // 방이 비었으면 삭제
        if (room.users.empty())
        {
            deadUsersByRoom.erase(it->roomName);
            readyUsersByRoom.erase(it->roomName);
            resolvedRooms_.erase(it->roomName);
            rooms.erase(it);
            roomDeleted = true;
        }
        else
        {
            // 남은 유저 리스트 문자열 생성
            for (size_t i = 0; i < room.users.size(); ++i)
            {
                if (i > 0) userListStr += ",";

                std::string uid = room.users[i];
                std::string nickname = Trim(clientHandler_.GetNicknameById(uid));

                int charIndex = 0;
                auto cit = room.characterSelections.find(uid);
                if (cit != room.characterSelections.end())
                    charIndex = cit->second;

                std::string team = "None";
                if (room.isCoopMode)
                {
                    auto tit = room.teamAssignments.find(uid);
                    if (tit != room.teamAssignments.end())
                        team = tit->second;
                }

                userListStr += nickname + ":" +
                    std::to_string(charIndex) + ":" + team;
            }
        }
    }

    // 방 삭제됨 → 전체 룸 리스트 브로드캐스트
    if (roomDeleted)
    {
        std::vector<std::shared_ptr<ClientInfo>> clients;
        { std::lock_guard<std::mutex> lock(server_.clientsMutex); clients = server_.GetClients(); }
        for (auto& c : clients)
        {
            SendRoomList(*c);
        }
        return;
    }

    // 방 남아있음 → 방 내부 갱신 브로드캐스트
    std::string refreshMsg =
        "REFRESH_ROOM_SUCCESS|" + roomName + "|" + userListStr + "\n";

    const auto members = GetUserIdsInRoom(roomName);
    std::lock_guard<std::mutex> lock(server_.clientsMutex);
    auto& clientsMap = clientHandler_.GetClientsMapLocked();

    for (const auto& uid : members)
    {
        auto it = clientsMap.find(uid);
        if (it != clientsMap.end())
        {
            QueueSend(it->second->socket,
                refreshMsg.c_str(),
                (int)refreshMsg.size(),
                0);
        }
    }
}

void RoomManager::RetryDeath(const Room& snapshot, const std::string& userId) {
    std::lock_guard<std::mutex> lock(roomsMutex);
    Room* room = FindRoomByName_NoLock(snapshot.roomName);
    if (!room || !room->isInGame || room->rewardsCommitted || room->matchId != snapshot.matchId) return;
    resolvedRooms_.erase(room->roomName);
    deadUsersByRoom[room->roomName].erase(userId);
}

bool RoomManager::WorldCommand(const std::string& id, const std::string& command, uint64_t match,
    uint64_t sequence, double x, double z) {
    std::lock_guard<std::mutex> lock(roomsMutex);
    for (auto& room : rooms) {
        if (std::find(room.users.begin(), room.users.end(), id) == room.users.end()) continue;
        if (!room.isInGame || room.matchId != match || room.rewardsPending) return false;
        if (command == "MATCH_READY") return room.world.Ready(id);
        if (command == "MOVE_INPUT") return room.world.Input(id, sequence, {x,z});
        if (command == "ATTACK") return room.world.Fire(id, sequence, x);
        if (command == "BALLOON") return room.world.Place(id, sequence);
    }
    return false;
}

std::string RoomManager::MapResponse(const std::string& id) {
    std::lock_guard<std::mutex> lock(roomsMutex);
    for (const auto& room : rooms) {
        if (!room.isInGame || std::find(room.users.begin(),room.users.end(),id)==room.users.end()) continue;
        // Per-client delivery, stable per-match spawns, no filesystem read or rerandomization.
        std::string map="MAP_DATA|"+room.mapName+"|", spawn, stats="CHAR_INFO|";
        for (const auto& row : room.world.map) {
            for (int cell : row) map+=std::to_string(cell)+",";
            map.back()=';';
        }
        map.back()='|';
        for (size_t i=0;i<room.users.size();++i) {
            const auto& uid=room.users[i];const auto& p=room.world.players.at(uid);const auto& c=room.combat.at(uid);
            spawn+=p.nickname+":"+std::to_string(int(p.position.x))+":"+std::to_string(int(p.position.z)+13)+":"+std::to_string(c.character)+",";
            stats+=p.nickname+","+std::to_string(c.character)+","+std::to_string(c.health)+","+std::to_string(c.attack)+"|";
        }
        spawn.pop_back();stats.pop_back();return map+spawn+"\n"+stats+"\n";
    }
    return {};
}

void RoomManager::BroadcastToMatch(const Room& match,const std::string& message) {
    std::vector<std::shared_ptr<ClientInfo>> recipients;
    {
        std::lock_guard<std::mutex> lock(roomsMutex);
        auto room=FindRoomByName_NoLock(match.roomName);
        if (!room || room->matchId!=match.matchId) return;
        std::lock_guard<std::mutex> clientsLock(server_.clientsMutex);
        const auto& clients=clientHandler_.GetClientsMapLocked();
        for (const auto& id:room->users) {auto p=clients.find(id);if(p!=clients.end())recipients.push_back(p->second);}
    }
    for(const auto& p:recipients)QueueSend(p->socket,message.data(),int(message.size()),0);
}

void RoomManager::TickWorld(double seconds) {
    std::vector<std::pair<std::shared_ptr<ClientInfo>,std::string>> sends;
    std::vector<std::pair<std::string,DeathResult>> deaths;
    {
        std::lock_guard<std::mutex> lock(roomsMutex);
        for(auto& room:rooms) {
            if(!room.isInGame || room.rewardsPending)continue;
            for(auto& p:room.world.players)
                if(std::find(room.users.begin(),room.users.end(),p.first)==room.users.end())room.world.RemovePlayer(p.first);
            room.world.Advance(seconds,[&](const std::string& attacker,const std::string& target,bool water) {
                auto hit=ApplyDamageLocked(room,attacker,target,water);if(!hit.accepted)return;
                const auto& nick=room.world.players.at(target).nickname;
                room.world.events.push_back("DAMAGE|"+nick+"|"+std::to_string(hit.damage)+"|"+std::to_string(hit.health)+"|"+
                    std::to_string(hit.matchId)+"|"+std::to_string(hit.revision)+"\n");
                if(hit.death.room)deaths.push_back({target,hit.death});
            });
            if(room.world.events.empty())continue;
            std::string framed;for(const auto& message:room.world.events)framed+=message;
            room.world.events.clear();
            std::lock_guard<std::mutex> clientsLock(server_.clientsMutex);
            const auto& clients=clientHandler_.GetClientsMapLocked();
            for(const auto& id:room.users){auto p=clients.find(id);if(p!=clients.end())sends.push_back({p->second,framed});}
        }
    }
    for(const auto& send:sends)QueueSend(send.first->socket,send.second.data(),int(send.second.size()),0);
    for(const auto& death:deaths) {
        try {clientHandler_.PublishDeath(death.first,death.second);}
        catch(const std::exception&) {std::cerr<<"Combat reward save failed; receipt retry pending\n";}
    }
}
