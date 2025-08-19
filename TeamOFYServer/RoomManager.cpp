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
//                message += "|";  // 방 구분자
//        }
//    }
//
//    message += "\n";
//    cout << "[룸리스트]" << message << endl;
//    int sendLen = send(client->socket, message.c_str(), (int)message.size(), 0);
//    if (sendLen == SOCKET_ERROR) {
//        std::cout << "[BroadcastRoomlist] 전송 실패: " << WSAGetLastError() << std::endl;
//    }
//    else {
//        std::cout << "[BroadcastRoomlist] 전송 완료: " << message;
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
                message += "|";  // 방 구분자
        }
    }

    message += "\n";
    cout << "[룸리스트]" << message << endl;
    int sendLen = send(client->socket, message.c_str(), (int)message.size(), 0);
    if (sendLen == SOCKET_ERROR) {
        std::cout << "[BroadcastRoomlist] 전송 실패: " << WSAGetLastError() << std::endl;
    }
    else {
        std::cout << "[BroadcastRoomlist] 전송 완료: " << message;
    }
}


// RoomManager 클래스 내
//bool RoomManager::CreateRoom(shared_ptr<ClientInfo> client, const string& roomName, const string& mapName, const string& password) {
//    if (client->id.empty()) {
//        cout << "[RoomManager] 로그인 안 된 클라이언트" << endl;
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
    bool isCoopMode) // 클라이언트에서 전달된 협동전 여부
{
    // 로그인 확인
    if (client->id.empty()) {
        cout << "[RoomManager] 로그인 안 된 클라이언트" << endl;
        string errorMsg = "ERROR|LOGIN_REQUIRED\n";
        send(client->socket, errorMsg.c_str(), (int)errorMsg.size(), 0);
        return false;
    }

    string userListStr;

    {
        lock_guard<mutex> lock(roomsMutex);

        // 새 룸 생성
        Room newRoom;
        newRoom.roomName = roomName;
        newRoom.mapName = mapName;
        newRoom.password = password;
        newRoom.isCoopMode = isCoopMode; // ← 협동전 여부 설정
        newRoom.users.push_back(client->id);

        rooms.push_back(newRoom);
        Room& createdRoom = rooms.back();

        // 유저 목록 문자열 생성
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

    // 비밀번호 및 협동전 여부 문자열
    string hasPasswordStr = password.empty() ? "NO_PASSWORD" : "HAS_PASSWORD";
    string coopStr = isCoopMode ? "1" : "0"; // 1 = 협동전, 0 = 개인전

    // 클라이언트에게 성공 메시지 전송
    string successMsg = "CREATE_ROOM_SUCCESS|" + roomName + "|" + mapName + "|CREATOR|"
        + userListStr + "|" + hasPasswordStr + "|" + coopStr + "\n";
    cout << successMsg << endl;
    send(client->socket, successMsg.c_str(), (int)successMsg.size(), 0);

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
        send(otherClient->socket, msgWithNewline.c_str(), (int)msgWithNewline.size(), 0);
        //cout << "[브로드캐스트 (제외)] → " << otherClient->ip << ":" << otherClient->port << " : " << msgWithNewline;
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
            cout << "[EnterRoom] 방 찾음: " << roomName << endl;

            if (room.password == password) {
                cout << "[EnterRoom] 비밀번호 일치" << endl;

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

                // 유저가 아직 방에 없으면 추가
                if (std::find(room.users.begin(), room.users.end(), client->id) == room.users.end()) {
                    room.users.push_back(client->id);
                    cout << "[EnterRoom] 사용자 " << client->id << " 입장 처리 완료" << endl;

                    // 캐릭터 선택 정보가 없으면 기본값 0으로 초기화
                    if (room.characterSelections.find(client->id) == room.characterSelections.end()) {
                        room.characterSelections[client->id] = 0;
                    }
                }
                else {
                    cout << "[EnterRoom] 사용자 " << client->id << " 이미 입장 상태" << endl;
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

                cout << "[EnterRoom] 유저 리스트: " << userListStr << endl;

                outResponse = "ENTER_ROOM_SUCCESS|" + roomName + "|" + mapName + "|" + userListStr + "\n";
                cout << "[Send to Client] " << outResponse;

                // 방에 있는 다른 유저들에게 갱신 메시지 전송
                string refreshMsg = "REFRESH_ROOM_SUCCESS|" + roomName + "|" + mapName + "|" + userListStr + "\n";
                cout << "[Broadcast to Others] " << refreshMsg;

                auto& clientsMap = clientHandler_.GetClientsMap();
                for (const auto& user : room.users) {
                    //if (user == client->id) continue;

                    auto it = clientsMap.find(user);
                    if (it != clientsMap.end()) {
                        send(it->second->socket, refreshMsg.c_str(), (int)refreshMsg.size(), 0);
                        cout << "[EnterRoom] 다른 유저 " << user << "에게 갱신 메시지 전송" << endl;
                    }
                }

                return true;
            }
            else {
                cout << "[EnterRoom] 비밀번호 불일치" << endl;
            }

            break; // 방은 찾았기 때문에 루프 종료
        }
    }

    // 실패 케이스 처리
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
    send(client.socket, message.c_str(), (int)message.size(), 0);

    
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

    string rawuserId = clientHandler_.GetIdByNickname(nickname);
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

// RoomManager 클래스 내부에 추가
void RoomManager::BroadcastToUserRoom(const std::string& senderId, const std::string& message) {
    std::lock_guard<std::mutex> lockRooms(roomsMutex);

    // senderId가 속한 방 찾기
    auto it = std::find_if(rooms.begin(), rooms.end(), [&](const Room& room) {
        return std::find(room.users.begin(), room.users.end(), senderId) != room.users.end();
        });

    if (it == rooms.end()) {
        std::cerr << "[BroadcastToUserRoom] senderId가 속한 방을 찾을 수 없음: " << senderId << std::endl;
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
                std::cerr << "[BroadcastToUserRoom] send 실패: " << WSAGetLastError() << " - userId: " << userId << std::endl;
            }
        }
        else {
            std::cerr << "[BroadcastToUserRoom] 클라이언트 검색 실패 - userId: " << userId << std::endl;
        }
    }
}

void RoomManager::BroadcastToRoomExcept(SOCKET exceptSocket, const std::string& message)
{
    std::lock_guard<std::mutex> lockRooms(roomsMutex);

    // excludedSocket이 속한 방 찾기
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
        std::cerr << "[GetUserRoomId] userId가 속한 방을 찾을 수 없음: " << userId << std::endl;
        return "";
    }

    return it->roomName;  // 방 구조체에 방 ID가 있다면 이렇게 리턴
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
            // 게임 종료 상태로 설정
            room.isInGame = false;
            std::cout << "[RoomManager] 방 '" << room.roomName << "' isInGame = false 로 변경됨 (게임 종료)\n";
        }
    }

    // 준비 신호 브로드캐스트
    {
        std::lock_guard<std::mutex> clientLock(server_.clientsMutex);
        auto& clientsMap = clientHandler_.GetClientsMap();

        for (const auto& uid : roomUsers)
        {
            if (uid == userId) continue; // 자신 제외

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
        // GAME_END 전송
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

        // 준비 상태 초기화
        {
            std::lock_guard<std::mutex> lock(roomsMutex);

            std::cout << "[DEBUG] HandleReadyToExit: 초기화 전 준비 유저 수: "
                << readyUsersByRoom[roomId].size() << ", 죽은 유저 수: "
                << deadUsersByRoom[roomId].size() << std::endl;

            readyUsersByRoom[roomId].clear();
            deadUsersByRoom[roomId].clear();

            std::cout << "[DEBUG] HandleReadyToExit: 초기화 완료. 준비 유저 수: "
                << readyUsersByRoom[roomId].size() << ", 죽은 유저 수: "
                << deadUsersByRoom[roomId].size() << std::endl;
        }

    }

    return allReady;
}
