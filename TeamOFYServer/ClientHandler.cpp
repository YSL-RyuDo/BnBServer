#include "ClientHandler.h"
#include "Server.h"

ClientHandler::ClientHandler(Server& server, UserManager& userManager, RoomManager& roomManager, MapManager& mapManager, Player& player)
    : server_(server), userManager_(userManager), roomManager_(roomManager), mapManager_(mapManager), player_(player){}

ClientHandler::~ClientHandler() {}

void ClientHandler::HandleClient(shared_ptr<ClientInfo> client) {
    char buffer[1025];

    while (true) {
        int recvLen = recv(client->socket, buffer, sizeof(buffer) - 1, 0);
        if (recvLen <= 0) break;

        buffer[recvLen] = '\0';
        string recvStr(buffer);
        ProcessMessages(client, recvStr);
    };
    closesocket(client->socket);
    server_.RemoveClient(client);
}

void ClientHandler::ProcessMessages(std::shared_ptr<ClientInfo> client, const std::string& recvStr)
{
    size_t start = 0;
    while (true)
    {
        size_t pos = recvStr.find('\n', start);
        string message;
        if (pos == string::npos)
        {
            message = recvStr.substr(start);
        }
        else {
            message = recvStr.substr(start, pos - start);
        }

        if (message.empty())
            break;

        if (message.rfind("MOVE|", 0) != 0 && message.rfind("MELODY_MOVE|", 0) != 0)
        {
            cout << "[수신] 클라이언트 메시지 - IP: " << client->port << ", 메시지: '" << message << "'" << endl;
        }
        string response;

        //if (message.rfind("LOGIN|", 0) == 0) {
        //    cout << "로그인 요청 감지" << endl;
        //    string loginData = message.substr(strlen("LOGIN|"));
        //    size_t commaPos = loginData.find(',');

        //    string id = (commaPos != string::npos) ? loginData.substr(0, commaPos) : "";
        //    string pw = (commaPos != string::npos) ? loginData.substr(commaPos + 1) : "";

        //    response = userManager_.CheckLogin(id, pw);

        //    string nick;
        //    if (response.rfind("LOGIN_SUCCESS|", 0) == 0) {
        //        size_t colonPos = response.find('|');
        //        if (colonPos != string::npos) {
        //            string userData = response.substr(colonPos + 1);
        //            stringstream ss(userData);
        //            string id, pw, nickname, levelStr, expStr;
        //            getline(ss, id, ',');
        //            getline(ss, pw, ',');
        //            getline(ss, nickname, ',');

        //            client->id = id;
        //            client->nickname = nickname;
        //        }

        //        {
        //            lock_guard<mutex> lock(server_.clientsMutex);

        //            for (auto& c : server_.GetClients()) {
        //                if (c->socket == client->socket) {
        //                    c->id = client->id;  // 덮어쓰기
        //                    c->nickname = client->nickname;
        //                    break;
        //                }
        //            }

        //            clientsMap[id] = client;  // nickname은 유일하므로 map은 그냥 덮어쓰기
        //        }
        //    }

        //    SendToClient(client, response);
        //}

        if (message.rfind("LOGIN|", 0) == 0) {
            cout << "로그인 요청 감지" << endl;
            string loginData = message.substr(strlen("LOGIN|"));
            size_t commaPos = loginData.find(',');

            string id = (commaPos != string::npos) ? loginData.substr(0, commaPos) : "";
            string pw = (commaPos != string::npos) ? loginData.substr(commaPos + 1) : "";

            // 이미 로그인한 유저인지 확인
            {
                lock_guard<mutex> lock(server_.clientsMutex);
                if (clientsMap.find(id) != clientsMap.end()) {
                    // 이미 접속중인 아이디
                    SendToClient(client, "ALREADY_LOGGED_IN|\n");
                    return;
                }
            }

            // 실제 로그인 검증
            response = userManager_.CheckLogin(id, pw);

            if (response.rfind("LOGIN_SUCCESS|", 0) == 0) {
                size_t colonPos = response.find('|');
                if (colonPos != string::npos) {
                    string userData = response.substr(colonPos + 1);
                    stringstream ss(userData);
                    string id, pw, nickname;
                    getline(ss, id, ',');
                    getline(ss, pw, ',');
                    getline(ss, nickname, ',');

                    client->id = id;
                    client->nickname = nickname;
                }

                {
                    lock_guard<mutex> lock(server_.clientsMutex);

                    for (auto& c : server_.GetClients()) {
                        if (c->socket == client->socket) {
                            c->id = client->id;
                            c->nickname = client->nickname;
                            break;
                        }
                    }

                    // 로그인 성공 시 맵에 등록
                    clientsMap[id] = client;
                }
            }

            SendToClient(client, response);
        }

        else if (message.rfind("REGISTER|", 0) == 0) {
            cout << "[ProcessMessages] 회원가입 요청 감지" << endl;

            string data = message.substr(strlen("REGISTER|"));
            stringstream ss(data);
            string id, pw, nick;
            if (getline(ss, id, ',') && getline(ss, pw, ',') && getline(ss, nick)) {
                cout << "[ProcessMessages] 회원가입 데이터 파싱 완료 - id: " << id << ", pw: " << pw << ", nick: " << nick << endl;
                response = userManager_.RegisterUser(id, pw, nick);
            }
            else {
                response = "REGISTER_ERROR|\n";
                cout << "[ProcessMessages] 회원가입 데이터 형식 오류" << endl;
            }
            SendToClient(client, response);
        }
        else if (message == "QUIT|")
        {
            cout << "[ProcessMessages] 클라이언트 종료 요청 수신" << endl;

            {
                lock_guard<mutex> lock(server_.clientsMutex);
                if (!client->id.empty()) {
                    clientsMap.erase(client->id); // 계정 연결 해제
                }
            }

            closesocket(client->socket);
            server_.RemoveClient(client);
            return;
        }

        else if (message.rfind("GET_USER_INFO|", 0) == 0)
        {
            string nickname = message.substr(strlen("GET_USER_INFO|"));

            userManager_.SendUserInfoByNickname(client, nickname);
        }
        else if (message == "GET_LOBBY_USER_LIST|")
        {
            cout << "접속 유저리스트 요청 감지" << endl;
            userManager_.BroadcastLobbyUserList();
        }
        else if (message == "GET_ROOM_LIST|")
        {
            cout << "방 생성 리스트 요청 감지" << endl;
            roomManager_.BroadcastRoomlist(client);
        }
        else if (message.rfind("LOBBY_MESSAGE|", 0) == 0)
        {
            cout << "[ProcessMessages] 로비 메시지 수신" << endl;

            string data = message.substr(strlen("LOBBY_MESSAGE|")); // "닉네임:메시지"
            size_t delimPos = data.find(':');

            if (delimPos != string::npos) {
                string nickname = data.substr(0, delimPos);
                string chatMsg = data.substr(delimPos + 1);

                userManager_.BroadcastLobbyChatMessage(nickname, chatMsg);
                response.clear();
            }
            else {
                cerr << "[ProcessMessages] 채팅 메시지 포맷 오류: " << data << endl;
            }
        }
        else if (message.rfind("LOGOUT|", 0) == 0) {
            {
                lock_guard<mutex> lock(server_.clientsMutex);
                if (!client->id.empty()) {
                    clientsMap.erase(client->id); // 접속중 목록에서 제거
                }
            }
            userManager_.LogoutUser(client);  // 필요하다면 유저 상태 갱신
            response = "LOGOUT_SUCCESS|\n";
}
        else if (message.rfind("CREATE_ROOM|", 0) == 0) {
            string data = message.substr(strlen("CREATE_ROOM|"));
            stringstream ss(data);
            string roomName, mapName, password, coopStr;
            bool isCoop = false;

            if (getline(ss, roomName, '|') && getline(ss, mapName, '|')) {
                if (!getline(ss, password, '|')) password = "";

                if (getline(ss, coopStr, '|')) {
                    for (auto& c : coopStr) c = tolower(c);
                    isCoop = (coopStr == "true" || coopStr == "1");
                }

                roomManager_.CreateRoom(client, roomName, mapName, password, isCoop);

                response.clear();
            }
            else {
                response = "CREATE_ROOM_FORMAT_ERROR\n";
            }
            }
        else if (message.rfind("ENTER_ROOM|", 0) == 0) {
            string data = message.substr(strlen("ENTER_ROOM|"));
            stringstream ss(data);
            string roomName, password, coopStr;
            getline(ss, roomName, '|');
            getline(ss, password, '|');
            getline(ss, coopStr, '|'); // 협동전 여부 추가

            bool isCoopMode = false;
            if (!coopStr.empty()) {
                isCoopMode = (coopStr == "true" || coopStr == "1");
            }

            string response;
            if (!roomManager_.EnterRoom(client, roomName, password, isCoopMode, response)) {
                
            }
            send(client->socket, response.c_str(), (int)response.size(), 0);
            }
        else if (message.rfind("ROOM_MESSAGE|", 0) == 0)
        {
            string data = message.substr(strlen("ROOM_MESSAGE|"));
            roomManager_.HandleRoomChatMessage(client, data);
        }
        else if (message.rfind("EXIT_ROOM|", 0) == 0) {
            roomManager_.ExitRoom(message);
        }
        else if (message.rfind("GET_CHARACTER|", 0) == 0)
        {
            std::string nickname = message.substr(strlen("GET_CHARACTER|"));
            nickname = Trim(nickname); // 공백 제거

            std::string userId = Trim(GetIdByNickname(nickname));
            if (userId.empty()) {
                std::cerr << "[GET_CHARACTER] 닉네임에 해당하는 ID 없음: " << nickname << std::endl;
                return;
            }

            std::vector<int> charList = userManager_.GetCharactersByUserId(userId);
            if (charList.empty()) {
                std::cerr << "[GET_CHARACTER] 해당 유저의 캐릭터 정보 없음: " << userId << std::endl;
                return;
            }

            std::string response = "CHARACTER_LIST|";
            for (size_t i = 0; i < charList.size(); ++i) {
                response += std::to_string(charList[i]);
                if (i != charList.size() - 1) response += ",";
            }
            response += "\n";

            send(client->socket, response.c_str(), (int)response.size(), 0);
        }
        else if (message.rfind("CHOOSE_CHARACTER|", 0) == 0)
        {
            string data = message.substr(strlen("CHOOSE_CHARACTER|"));
            roomManager_.HandleCharacterChoice(*client, data);
        }
        
        else if (message.rfind("TEAMCHANGE|", 0) == 0)
        {
            string nickname = message.substr(strlen("TEAMCHANGE|"));

            string userId = GetIdByNickname(nickname);
            if (userId.empty())
                return;

            string roomId = roomManager_.GetUserRoomId(userId);
            if (roomId.empty())
                return;

            Room* room = roomManager_.FindRoomByName(roomId);
            if (!room)
                return;

            if (!room->isCoopMode)
                return;

            auto itTeam = room->teamAssignments.find(userId);
            if (itTeam == room->teamAssignments.end())
                return;

            string& currentTeam = itTeam->second;
            if (currentTeam == "None")
                return;

            string newTeam;
            if (currentTeam == "Blue")
                newTeam = "Red";
            else if (currentTeam == "Red")
                newTeam = "Blue";
            currentTeam = newTeam;

            string userListStr;
            auto usersInRoom = roomManager_.GetUserIdsInRoom(roomId);
            for (size_t i = 0; i < usersInRoom.size(); ++i)
            {
                string uid = usersInRoom[i];
                string nick = Trim(GetNicknameById(uid));

                int characterIndex = 0;
                auto itChar = room->characterSelections.find(uid);
                if (itChar != room->characterSelections.end())
                    characterIndex = itChar->second;

                string team = "None";
                auto itTeam2 = room->teamAssignments.find(uid);
                if (itTeam2 != room->teamAssignments.end())
                    team = itTeam2->second;

                if (i > 0) userListStr += ",";
                userListStr += nick + ":" + std::to_string(characterIndex) + ":" + team;
            }

            string response = "REFRESH_ROOM_SUCCESS|" + room->roomName + "|" + room->mapName + "|" + userListStr + "\n";
            cout << response << endl;
            roomManager_.BroadcastToUserRoom(userId, response);

            string serverMsg = nickname + " has been changed to " + newTeam;
            roomManager_.SendServerMessageToRoom(room->roomName, serverMsg);

        }

        else if (message.rfind("START_GAME|", 0) == 0)
        {
            string roomName = message.substr(strlen("START_GAME|"));
            vector<string> userList;

            bool isCoop = false;
            {
                Room* room = roomManager_.FindRoomByName(roomName);
                if (room == nullptr) {
                    string failMsg = "START_GAME_FAIL|ROOM_NOT_FOUND\n";
                    send(client->socket, failMsg.c_str(), (int)failMsg.size(), 0);
                    return;
                }
                isCoop = room->isCoopMode;
            }

            if (!isCoop)
            {
                // 개인전 처리
                if (!roomManager_.TryStartGame(roomName, userList))
                {
                    string failMsg = "START_GAME_FAIL|";
                    if (userList.empty())
                        failMsg += "ROOM_NOT_FOUND\n";
                    else
                        failMsg += "NOT_ENOUGH_PLAYERS\n";

                    send(client->socket, failMsg.c_str(), (int)failMsg.size(), 0);
                    return;
                }
            }
            else
            {
                // 협동전 처리
                vector<string> blueTeam;
                vector<string> redTeam;

                if (!roomManager_.TryStartCoopGame(roomName, userList, blueTeam, redTeam))
                {
                    string failMsg = "START_GAME_FAIL|";
                    if (userList.empty())
                        failMsg += "ROOM_NOT_FOUND222\n";
                    else if (userList.size() < 4)
                        failMsg += "NOT_ENOUGH_PLAYERS\n";
                    else
                        failMsg += "INVALID_TEAM_COMPOSITION\n";

                    send(client->socket, failMsg.c_str(), (int)failMsg.size(), 0);
                    return;
                }
            }

            string response = "START_GAME_SUCCESS|" + roomName + "\n";
            send(client->socket, response.c_str(), (int)response.size(), 0);

            string startMsg = "GAME_START|" + roomName + "\n";
            for (const auto& user : userList)
            {
                string userId = GetIdByNickname(user);
                auto it = clientsMap.find(userId);
                if (it != clientsMap.end())
                {
                    send(it->second->socket, startMsg.c_str(), (int)startMsg.size(), 0);
                }
            }
        } 
        else if (message.rfind("GET_MAP|", 0) == 0)
        {
            std::string data = message.substr(strlen("GET_MAP|"));
            std::stringstream ss(data);
            std::string roomName, mapName;

            if (!getline(ss, roomName, '|') || !getline(ss, mapName))
            {
                std::cerr << "[GET_MAP] 메시지 형식 오류: " << message << std::endl;
                return;
            }

            if (!characterStatsManager_.LoadFromName("CharStat")) {
                std::cerr << "[GET_MAP] CharStat.csv 로딩 실패" << std::endl;
                return;
            }

            auto result = mapManager_.LoadMapByNameWithSpawns(mapName);
            auto& mapData = result.first;
            auto& allSpawnPoints = result.second;

            Room* room = roomManager_.FindRoomByName(roomName);
            for (const auto& userId : room->users) {
                string nickname = Trim(GetNicknameById(userId));
            }

            size_t userCount = room->users.size();

            if (userCount > allSpawnPoints.size())
            {
                std::cerr << "[GET_MAP] 스폰 좌표 부족" << std::endl;
                return;
            }

            std::string mapDataStr;
            for (const auto& row : mapData)
            {
                for (int cell : row)
                {
                    mapDataStr += std::to_string(cell) + ",";
                }
                if (!row.empty()) mapDataStr.pop_back();
                mapDataStr += ";";
            }
            if (!mapData.empty()) mapDataStr.pop_back();

            std::vector<size_t> indices(allSpawnPoints.size());
            std::iota(indices.begin(), indices.end(), 0);

            std::random_device rd;
            std::mt19937 g(rd());
            std::shuffle(indices.begin(), indices.end(), g);

            std::string spawnStr;
            for (size_t i = 0; i < userCount; ++i) {
                const std::string& userId = room->users[i];
                string nickname = Trim(GetNicknameById(userId));

                int x = std::get<0>(allSpawnPoints[indices[i]]);
                int y = std::get<1>(allSpawnPoints[indices[i]]);
                int charIndex = 0;

                auto it = room->characterSelections.find(userId);
                if (it != room->characterSelections.end())
                    charIndex = it->second;

                spawnStr += nickname + ":" + std::to_string(x) + ":" + std::to_string(y) + ":" + std::to_string(charIndex) + ",";
            }
            if (!spawnStr.empty()) spawnStr.pop_back();

            std::string charInfoStr = "CHAR_INFO|";
            for (size_t i = 0; i < userCount; ++i) {
                const std::string& userId = room->users[i];
                std::string nickname = Trim(GetNicknameById(userId));

                int charIndex = 0;
                auto itCharSel = room->characterSelections.find(userId);
                if (itCharSel != room->characterSelections.end())
                    charIndex = itCharSel->second;

                const CharStats* stats = characterStatsManager_.GetStats(charIndex);
                int health = stats ? stats->health : 0;
                int attack = stats ? stats->attack : 0;

                charInfoStr += nickname + "," + std::to_string(charIndex) + "," +
                    std::to_string(health) + "," + std::to_string(attack);

                if (i != userCount - 1)
                    charInfoStr += "|";
            }
            charInfoStr += "\n";


            std::cout << "[서버 캐릭터 전송] " << charInfoStr;
            std::string response = "MAP_DATA|" + mapName + "|" + mapDataStr + "|" + spawnStr + "\n";
            std::cout << "[서버 MAP_DATA 전송] " << response;
            std::lock_guard<std::mutex> lock(server_.GetClientsMutex());
            auto& clientsMap = GetClientsMap();

            std::vector<std::string> userList;
            for (const auto& userId : room->users) {
                std::string nickname = Trim(GetNicknameById(userId));
                userList.push_back(nickname);
            }

            for (const auto& nickname : userList)
            {
                std::string userId = Trim(GetIdByNickname(nickname));
                auto it = clientsMap.find(userId);
                if (it != clientsMap.end())
                {
                    send(it->second->socket, response.c_str(), (int)response.size(), 0);
                    send(it->second->socket, charInfoStr.c_str(), (int)charInfoStr.size(), 0);
                }
            }

        }
        else if (message.rfind("GET_BALLOON|", 0) == 0)
        {
            string nickname = message.substr(strlen("GET_BALLOON|"));
            nickname = Trim(nickname);

            string userId = Trim(GetIdByNickname(nickname));
            if (userId.empty()) {
                std::cerr << "[GET_EMO] 닉네임에 해당하는 ID 없음: " << nickname << std::endl;
                return;
            }

            auto balloonType = userManager_.GetBalloonByUserId(userId);
            std::string response = "BALLOON_LIST|" + std::to_string(balloonType) + "\n";
            send(client->socket, response.c_str(), (int)response.size(), 0);
        }
        else if (message.rfind("GET_EMO|", 0) == 0) {
            std::string nickname = message.substr(strlen("GET_EMO|"));
            nickname = Trim(nickname);

            std::string userId = Trim(GetIdByNickname(nickname));
            if (userId.empty()) {
                std::cerr << "[GET_EMO] 닉네임에 해당하는 ID 없음: " << nickname << std::endl;
                return;
            }

            auto emoList = userManager_.GetEmotionsByUserId(userId);

            std::string response = "EMO_LIST|";
            for (size_t i = 0; i < emoList.size(); ++i) {
                response += std::to_string(emoList[i]);
                if (i != emoList.size() - 1) response += ",";
            }
            response += "\n";
            send(client->socket, response.c_str(), (int)response.size(), 0);
        }
        else if (message.rfind("EMO_CLICK|", 0) == 0) {
            std::string data = message.substr(strlen("EMO_CLICK|"));
            std::stringstream ss(data);
            std::string nickname, emoIndexStr;

            if (!getline(ss, nickname, '|') || !getline(ss, emoIndexStr)) {
                std::cerr << "[EMO_CLICK] 메시지 형식 오류: " << message << std::endl;
                return;
            }

            std::string senderId = Trim(GetIdByNickname(nickname));
            if (senderId.empty()) {
                std::cerr << "[EMO_CLICK] 닉네임에서 ID 변환 실패: " << nickname << std::endl;
                return;
            }

            std::string broadcastMsg = "EMO_CLICK|" + nickname + "|" + emoIndexStr + "\n";

            roomManager_.BroadcastToUserRoom(senderId, broadcastMsg);
        }
        else if (message.rfind("MOVE|", 0) == 0)
        {
            std::string data = message.substr(strlen("MOVE|")); // username|x,z

            size_t delimiterPos = data.find('|');
            if (delimiterPos == std::string::npos) return;

            std::string username = data.substr(0, delimiterPos);
            std::string coords = data.substr(delimiterPos + 1);

            size_t commaPos = coords.find(',');
            if (commaPos == std::string::npos) return;

            float x = std::stof(coords.substr(0, commaPos));
            float z = std::stof(coords.substr(commaPos + 1));
            
            bool updated = player_.UpdatePlayerPosition(username, x, z);
            if (updated)
            {
                auto pos = player_.GetPlayerPosition(username);
                std::string resultMsg = "MOVE_RESULT|" + username + "," +
                    std::to_string(pos.first) + "," + std::to_string(pos.second);
                
                // 보내는 클라이언트 소켓을 제외하고 같은 방 유저들에게 전송
                roomManager_.BroadcastMessageExcept(client->socket, resultMsg);
            }
        }

        else if (message.rfind("WEAPON_ATTACK|", 0) == 0)
        {
            std::string data = message.substr(strlen("WEAPON_ATTACK|"));

            // 1. 첫 번째 구분자 '|'
            size_t first = data.find('|');
            if (first == std::string::npos) return;
            std::string nickname = data.substr(0, first);

            // 2. 두 번째 구분자 '|'
            size_t second = data.find('|', first + 1);
            if (second == std::string::npos) return;
            std::string charIndexStr = data.substr(first + 1, second - first - 1);

            // 3. 세 번째 구분자 '|'
            size_t third = data.find('|', second + 1);
            if (third == std::string::npos) return;
            std::string positionStr = data.substr(second + 1, third - second - 1);

            // 4. 네 번째 구분자 '|'
            size_t fourth = data.find('|', third + 1);

            std::string rotYStr;
            std::string laserLengthStr;

            if (fourth == std::string::npos)
            {
                rotYStr = data.substr(third + 1);
                laserLengthStr = "";
            }
            else
            {
                rotYStr = data.substr(third + 1, fourth - third - 1);
                laserLengthStr = data.substr(fourth + 1);
            }

            std::string forwardMsg;
            if (!laserLengthStr.empty())
            {
                forwardMsg = "WEAPON_ATTACK|" + nickname + "|" + charIndexStr + "|" + positionStr + "|" + rotYStr + "|" + laserLengthStr + "\n";
            }
            else
            {
                forwardMsg = "WEAPON_ATTACK|" + nickname + "|" + charIndexStr + "|" + positionStr + "|" + rotYStr + "\n";
            }

            roomManager_.BroadcastToRoomExcept(client->socket, forwardMsg);

            std::cout << "[Server] WEAPON_ATTACK 처리 완료 from " << nickname << std::endl;
            }

        else if (message.rfind("MELODY_MOVE|", 0) == 0)
        {
            std::string data = message.substr(strlen("MELODY_MOVE|"));

            size_t first = data.find('|');
            if (first == std::string::npos) return;
            std::string nickname = data.substr(0, first);

            size_t second = data.find('|', first + 1);
            if (second == std::string::npos) return;
            std::string posStr = data.substr(first + 1, second - first - 1);

            std::string rotYStr = data.substr(second + 1);

            std::string forwardMsg = "MELODY_MOVE|" + nickname + "|" + posStr + "|" + rotYStr + "\n";
            roomManager_.BroadcastToRoomExcept(client->socket, forwardMsg);

            std::cout << "[Server] Melody 이동 정보 브로드캐스트 (" << nickname << "): " << posStr << ", " << rotYStr << std::endl;
            }
        else if (message.rfind("MELODY_DESTROY|", 0) == 0)
        {
            std::string nickname = message.substr(strlen("MELODY_DESTROY|"));
            std::string forwardMsg = "MELODY_DESTROY|" + nickname + "\n";

            roomManager_.BroadcastToUserRoom(client->id, forwardMsg);
            std::cout << "[Server] Melody 제거 요청 처리 완료 from " << nickname << std::endl;
        }
        else if (message.rfind("HITWALL|", 0) == 0)
        {
            std::string wallName = message.substr(strlen("HITWALL|"));

            std::string forwardMsg = "DESTROYWALL|" + wallName + "\n";
            roomManager_.BroadcastToUserRoom(client->id, forwardMsg);

            std::cout << "[Server] Wall 파괴 브로드캐스트: " << wallName << std::endl;
        }
        else if (message.rfind("DESTROY_SPELL|", 0) == 0)
        {
            std::string spellName = message.substr(strlen("DESTROY_SPELL|"));
            roomManager_.BroadcastToUserRoom(client->id, message);
            std::cout << "[Server] 스펠 제거 패킷 브로드캐스트: " << spellName << "\n";
        }
        else if (message.rfind("DESTROY_BLOCK|", 0) == 0)
        {
            std::string blockName = message.substr(strlen("DESTROY_BLOCK|"));
            blockName = Trim(blockName);

            if (blockName.empty())
                return;

            std::cout << "[서버] 블록 파괴 요청 수신: " << blockName << std::endl;

            std::string broadcastMsg = "DESTROY_BLOCK|" + blockName + "\n";

            Room* room = roomManager_.GetRoomByUserId(client->id);
            if (room != nullptr)
            {
                roomManager_.BroadcastToUserRoom(client->id, broadcastMsg);
                std::cout << "[서버] DESTROY_BLOCK 브로드캐스트: " << blockName << std::endl;
            }
        }

        else if (message.rfind("HIT|", 0) == 0)
        {
            // 패킷 구조: HIT|weaponIndex|attackerNick|targetNick
            std::string data = message.substr(strlen("HIT|"));

            size_t delim1 = data.find('|');
            if (delim1 == std::string::npos) return;
            int weaponIndex = std::stoi(data.substr(0, delim1));

            size_t delim2 = data.find('|', delim1 + 1);
            if (delim2 == std::string::npos) return;
            std::string attackerNick = data.substr(delim1 + 1, delim2 - (delim1 + 1));

            size_t delim3 = data.find('|', delim2 + 1);
            std::string targetNick;
            if (delim3 == std::string::npos)
                targetNick = data.substr(delim2 + 1);
            else
                targetNick = data.substr(delim2 + 1, delim3 - (delim2 + 1));

            // 닉네임 → 유저 ID 변환
            std::string attackerId = GetIdByNickname(attackerNick);
            std::string targetId = GetIdByNickname(targetNick);

            // 이제 attackerId와 targetId로 룸 정보 및 팀 체크 가능
            Room* room = roomManager_.GetRoomByUserId(attackerId);
            if (room != nullptr)
            {
                std::string attackerTeam = "None";
                std::string targetTeam = "None";

                if (room->isCoopMode)
                {
                    for (const auto& userId : room->users)
                    {
                        std::string team = "None";
                        auto teamIt = room->teamAssignments.find(userId);
                        if (teamIt != room->teamAssignments.end())
                            team = teamIt->second;

                        if (userId == attackerId) attackerTeam = team;
                        if (userId == targetId)   targetTeam = team;
                    }

                    // 같은 팀이면 데미지 무효
                    if (attackerTeam != "None" && attackerTeam == targetTeam)
                    {
                        std::cout << "[서버] " << attackerNick << " → " << targetNick
                            << " (같은 팀, 데미지 무효)\n";
                        return;
                    }
                }

                // 데미지 전송
                int damage = userManager_.GetAttackByIndex(weaponIndex);
                std::string resultMsg = "DAMAGE|" + targetNick + "|" + std::to_string(damage) + "\n";
                roomManager_.BroadcastToUserRoom(attackerId, resultMsg);

                std::cout << "[서버] " << attackerNick << " → " << targetNick
                    << " 에게 " << damage << " 데미지 전달\n";
            }
        }
        else if (message.rfind("PLACE_BALLOON|", 0) == 0)
        {
            std::string data = message.substr(strlen("PLACE_BALLOON|"));

            size_t firstBar = data.find('|');
            if (firstBar == std::string::npos) return;

            std::string nickname = data.substr(0, firstBar);
            std::string rest = data.substr(firstBar + 1);

            size_t secondBar = rest.find('|');
            if (secondBar == std::string::npos) return;

            std::string posStr = rest.substr(0, secondBar); // "x,z"
            std::string typeStr = rest.substr(secondBar + 1); // 풍선 타입

            size_t commaPos = posStr.find(',');
            if (commaPos == std::string::npos) return;

            float x = std::stof(posStr.substr(0, commaPos));
            float z = std::stof(posStr.substr(commaPos + 1));
            int type = std::stoi(typeStr);

            x = std::round(x * 100.0f) / 100.0f;
            z = std::round(z * 100.0f) / 100.0f;

            std::ostringstream oss;
            oss << "PLACE_BALLOON_RESULT|" << nickname << "|"
                << x << "," << z << "|" << type << "\n";  // <-- 여기 \n


            std::string result = oss.str();
            string senderId = Trim(GetIdByNickname(nickname));
            roomManager_.BroadcastToUserRoom(senderId, result);


        }
        else if (message.rfind("REMOVE_BALLOON|", 0) == 0)
        {
            std::string data = message.substr(strlen("REMOVE_BALLOON|"));

            size_t firstBarPos = data.find('|');
            if (firstBarPos == std::string::npos) return;

            size_t secondBarPos = data.find('|', firstBarPos + 1);
            if (secondBarPos == std::string::npos) return;

            std::string nickname = data.substr(0, firstBarPos);
            std::string posStr = data.substr(firstBarPos + 1, secondBarPos - firstBarPos - 1); // x,z
            std::string typeStr = data.substr(secondBarPos + 1); // 타입

            size_t commaPos = posStr.find(',');
            if (commaPos == std::string::npos) return;

            float x = std::stof(posStr.substr(0, commaPos));
            float z = std::stof(posStr.substr(commaPos + 1));

            int balloonType = std::stoi(typeStr);

            x = std::round(x * 100.0f) / 100.0f;
            z = std::round(z * 100.0f) / 100.0f;

            std::ostringstream removeMsg;
            removeMsg << "REMOVE_BALLOON|" << nickname << "|" << x << "," << z << "\n";

            std::string result = removeMsg.str();

            std::string senderId = Trim(GetIdByNickname(nickname));
            roomManager_.BroadcastToUserRoom(senderId, result);

            std::cout << "[Server] REMOVE_BALLOON broadcast: " << result;

            std::ostringstream waterSpread;
            waterSpread << "WATER_SPREAD|" << nickname << "|" << x << "," << z << "|" << balloonType << "|";

            const int range = 2;
            for (int dx = -range; dx <= range; dx++)
            {
                if (dx == 0) continue;
                waterSpread << (x + dx) << "," << z << ";";
            }
            for (int dz = -range; dz <= range; dz++)
            {
                waterSpread << x << "," << (z + dz) << ";";
            }

            std::string waterMsg = waterSpread.str();
            if (waterMsg.back() == ';') waterMsg.pop_back();
            waterMsg += "\n";

            roomManager_.BroadcastToUserRoom(senderId, waterMsg);
            std::cout << "[Server] WATER_SPREAD broadcast: " << waterMsg;
        }
        else if (message.rfind("WATER_HIT|", 0) == 0)
        {
            std::string data = message.substr(strlen("WATER_HIT|"));

            size_t barPos = data.find('|');
            if (barPos == std::string::npos) return;

            std::string hitNickname = data.substr(0, barPos);
            std::string damageStr = data.substr(barPos + 1);

            int damage = std::stoi(damageStr);

            std::string hitUserId = Trim(GetIdByNickname(hitNickname));

            std::ostringstream oss;
            oss << "PLAYER_HIT|" << hitNickname << "|" << damage << "\n";

            std::string result = oss.str();

            roomManager_.BroadcastToUserRoom(hitUserId, result);
            std::cout << "[Server] PLAYER_HIT broadcast: " << result;
        }
        else if (message.rfind("DEAD|", 0) == 0)
        {
            // 1. 죽은 유저 정보
            std::string deadNickname = message.substr(strlen("DEAD|"));
            std::string deadUserId = Trim(GetIdByNickname(deadNickname));
            std::string roomId = roomManager_.GetUserRoomId(deadUserId);
            if (roomId.empty()) return;

            // 2. 중복 처리 방지
            auto& deadSet = roomManager_.deadUsersByRoom[roomId];
            if (deadSet.find(deadUserId) != deadSet.end()) return;
            deadSet.insert(deadUserId);

            // 3. PLAYER_DEAD 패킷 브로드캐스트
            std::ostringstream deadMsg;
            deadMsg << "PLAYER_DEAD|" << deadNickname << "\n";
            roomManager_.BroadcastToUserRoom(deadUserId, deadMsg.str());
            std::cout << "[Server] PLAYER_DEAD broadcast: " << deadMsg.str();

            // 4. 방 객체 가져오기
            Room* room = roomManager_.FindRoomByName(roomId);
            if (!room) return;

            // 5. 방 전체 유저 ID
            std::vector<std::string> roomUserIds = roomManager_.GetUserIdsInRoom(roomId);

            // 6. 살아있는 유저와 팀별 목록
            std::unordered_map<std::string, std::vector<std::string>> aliveTeams;
            for (const auto& userId : roomUserIds)
            {
                if (deadSet.find(userId) != deadSet.end()) continue;

                std::string team = "None";
                auto teamIt = room->teamAssignments.find(userId);
                if (teamIt != room->teamAssignments.end())
                    team = teamIt->second;

                std::string nickname = Trim(GetNicknameById(userId));
                aliveTeams[team].push_back(nickname);
            }

            // --- 개인전 처리 ---
            if (!room->isCoopMode)
            {
                if (!aliveTeams.empty() && aliveTeams.begin()->second.size() == 1)
                {
                    std::string winnerNickname = aliveTeams.begin()->second[0];

                    std::ostringstream winMsg;
                    winMsg << "WIN|" << winnerNickname << "\n";
                    roomManager_.BroadcastToUserRoom(deadUserId, winMsg.str());
                    std::cout << "[Server] WIN packet sent to: " << winnerNickname << "\n";

                    // 보상 처리
                    std::ostringstream rewardMsg;
                    rewardMsg << "REWARD_RESULT";

                    for (const auto& userId : roomUserIds)
                    {
                        std::string nickname = Trim(GetNicknameById(userId));
                        bool isWinner = (nickname == winnerNickname);

                        int gainedExp = isWinner ? 50 : 30;
                        int gainedCoin0 = isWinner ? 500 : 200;
                        int gainedCoin1 = isWinner ? 5 : 0;

                        UserProfile& profile = userManager_.GetUserProfileById(userId);
                        profile.exp += gainedExp;

                        while (profile.exp >= profile.level * 100)
                        {
                            profile.exp -= profile.level * 100;
                            profile.level++;
                        }

                        profile.money0 += gainedCoin0;
                        profile.money1 += gainedCoin1;

                        int charIndex = 0;
                        auto itChar = room->characterSelections.find(userId);
                        if (itChar != room->characterSelections.end())
                            charIndex = itChar->second;

                        userManager_.UpdateWinLoss(userId, isWinner, charIndex);
                        userManager_.SaveUserWinLossStats(userId);
                        userManager_.SaveUserProfile(userId);

                        rewardMsg << "|" << nickname
                            << ",level:" << profile.level
                            << ",exp:" << gainedExp
                            << ",money0:" << gainedCoin0
                            << ",money1:" << gainedCoin1;
                    }

                    rewardMsg << "\n";
                    roomManager_.BroadcastToUserRoom(deadUserId, rewardMsg.str());
                    std::cout << "[Server] REWARD_RESULT broadcast: " << rewardMsg.str();
                }
            }
            // --- 팀전 처리 ---
            else
            {
                if (aliveTeams.size() == 1)
                {
                    auto it = aliveTeams.begin();
                    std::string survivingTeam = it->first;
                    std::vector<std::string> survivingNicknames = it->second;

                    // TEAM_WIN 패킷 생성 (팀 이름, 닉네임1, 닉네임2...)
                    std::ostringstream teamWinMsg;
                    teamWinMsg << "TEAM_WIN|" << survivingTeam;
                    for (const auto& nick : survivingNicknames)
                        teamWinMsg << "," << nick;
                    teamWinMsg << "\n";

                    roomManager_.BroadcastToUserRoom(deadUserId, teamWinMsg.str());
                    std::cout << "[Server] TEAM_WIN broadcast: " << teamWinMsg.str();

                    // 보상 처리
                    std::ostringstream rewardMsg;
                    rewardMsg << "REWARD_RESULT";

                    for (const auto& userId : roomUserIds)
                    {
                        std::string team = "None";
                        auto teamIt = room->teamAssignments.find(userId);
                        if (teamIt != room->teamAssignments.end())
                            team = teamIt->second;

                        std::string nickname = Trim(GetNicknameById(userId));
                        bool isWinner = (team == survivingTeam);

                        int gainedExp = isWinner ? 50 : 30;
                        int gainedCoin0 = isWinner ? 500 : 200;
                        int gainedCoin1 = isWinner ? 5 : 0;

                        UserProfile& profile = userManager_.GetUserProfileById(userId);
                        profile.exp += gainedExp;

                        while (profile.exp >= profile.level * 100)
                        {
                            profile.exp -= profile.level * 100;
                            profile.level++;
                        }

                        profile.money0 += gainedCoin0;
                        profile.money1 += gainedCoin1;

                        int charIndex = 0;
                        auto itChar = room->characterSelections.find(userId);
                        if (itChar != room->characterSelections.end())
                            charIndex = itChar->second;

                        userManager_.UpdateWinLoss(userId, isWinner, charIndex);
                        userManager_.SaveUserWinLossStats(userId);
                        userManager_.SaveUserProfile(userId);

                        rewardMsg << "|" << nickname
                            << ",level:" << profile.level
                            << ",exp:" << gainedExp
                            << ",money0:" << gainedCoin0
                            << ",money1:" << gainedCoin1;
                    }

                    rewardMsg << "\n";
                    roomManager_.BroadcastToUserRoom(deadUserId, rewardMsg.str());
                    std::cout << "[Server] REWARD_RESULT broadcast: " << rewardMsg.str();
                }
            }
        }

        else if (message.rfind("READY_TO_EXIT|", 0) == 0)
        {
            std::string nickname = message.substr(strlen("READY_TO_EXIT|"));
            std::string userId = Trim(GetIdByNickname(nickname));
            std::string roomId = roomManager_.GetUserRoomId(userId);

            if (roomId.empty())
                return;

            roomManager_.HandleReadyToExit(userId, roomId, client->socket);
        }

        if (pos == std::string::npos)
            break;

            start = pos + 1;
    }
}

bool ClientHandler::SendToClient(std::shared_ptr<ClientInfo> client, const std::string& response)
{
    int sendLen = send(client->socket, response.c_str(), static_cast<int>(response.size()), 0);
    if (sendLen == SOCKET_ERROR)
    {
        std::cout << "[SendToClient] send 오류 - IP: " << client->ip << ", 포트: " << client->port
            << ", 오류 코드: " << WSAGetLastError() << std::endl;
        return false;
    }

    std::cout << "[SendToClient] 클라이언트 응답 전송 - IP: " << client->ip << ":" << client->port
        << " -> " << response;
    return true;
} 

bool ClientHandler::GetUserPositionById(const std::string& userId, std::pair<float, float>& outPos)
{
    auto it = userPositions.find(userId);
    if (it == userPositions.end())
        return false;

    outPos = it->second;
    return true;
}