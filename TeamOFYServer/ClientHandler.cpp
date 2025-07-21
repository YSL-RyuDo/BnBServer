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

        if (message.rfind("MOVE|", 0) != 0) 
        {
            cout << "[수신] 클라이언트 메시지 - IP: " << client->ip << ", 메시지: '" << message << "'" << endl;
        }
        string response;

        if (message.rfind("LOGIN|", 0) == 0) {
            cout << "로그인 요청 감지" << endl;
            string loginData = message.substr(strlen("LOGIN|"));
            size_t commaPos = loginData.find(',');

            string id = (commaPos != string::npos) ? loginData.substr(0, commaPos) : "";
            string pw = (commaPos != string::npos) ? loginData.substr(commaPos + 1) : "";

            response = userManager_.CheckLogin(id, pw);

            string nick;
            if (response.rfind("LOGIN_SUCCESS|", 0) == 0) {
                size_t colonPos = response.find('|');
                if (colonPos != string::npos) {
                    string userData = response.substr(colonPos + 1);
                    stringstream ss(userData);
                    string id, pw, nickname, levelStr, expStr;
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
                            c->id = client->id;  // 덮어쓰기
                            c->nickname = client->nickname;
                            break;
                        }
                    }

                    clientsMap[id] = client;  // nickname은 유일하므로 map은 그냥 덮어쓰기
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
            userManager_.LogoutUser(client);
            response.clear();
        }
        else if (message.rfind("CREATE_ROOM|", 0) == 0) {
            string data = message.substr(strlen("CREATE_ROOM|"));
            stringstream ss(data);
            string roomName, mapName, password;

            if (getline(ss, roomName, '|') && getline(ss, mapName, '|')) {
                if (!getline(ss, password)) password = "";

                // 룸 매니저에 위임
                roomManager_.CreateRoom(client, roomName, mapName, password);

                response.clear();
            }
            else {
                response = "CREATE_ROOM_FORMAT_ERROR\n";
            }
        }
        else if (message.rfind("ENTER_ROOM|", 0) == 0) {
            string data = message.substr(strlen("ENTER_ROOM|"));
            stringstream ss(data);
            string roomName, password;
            getline(ss, roomName, '|');
            getline(ss, password);

            string response;
            if (!roomManager_.EnterRoom(client, roomName, password, response)) {
            }
            send(client->socket, response.c_str(), (int)response.size(), 0);
        }
        
        else if (message.rfind("ROOM_MESSAGE|", 0) == 0)
        {
            // 기존 처리 코드를 RoomManager에 넘기기
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
        else if (message.rfind("START_GAME|", 0) == 0)
        {
            string roomName = message.substr(strlen("START_GAME|"));
            vector<string> userList;

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

            string response = "START_GAME_SUCCESS\n";
            send(client->socket, response.c_str(), (int)response.size(), 0);

            string startMsg = "GAME_START|" + roomName + "\n";
            for (const auto& user : userList)
            {
                string userId = GetIdByNickname(user);
                //string userId = Trim(rawuserId);
                auto it = clientsMap.find(userId);
                if (it != clientsMap.end())
                {
                    send(it->second->socket, startMsg.c_str(), (int)startMsg.size(), 0);
                }
            }
        }

        else if (message.rfind("GET_MAP|", 0) == 0)
        {
            // message 예: GET_MAP|roomName|mapName
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

            // 맵 데이터와 스폰 좌표 불러오기
            auto result = mapManager_.LoadMapByNameWithSpawns(mapName);
            auto& mapData = result.first;
            auto& allSpawnPoints = result.second;

            Room* room = roomManager_.FindRoomByName(roomName);

            for (const auto& userId : room->users) {
                string nickname = Trim(GetNicknameById(userId)); // 변환
                // nickname으로 전송 메시지 구성
            }

            size_t userCount = room->users.size();

            if (userCount > allSpawnPoints.size())
            {
                std::cerr << "[GET_MAP] 스폰 좌표 부족" << std::endl;
                return;
            }

            // mapData 문자열 직렬화
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

            // 스폰 좌표 인덱스 배열 생성 및 셔플 (랜덤 할당)
            std::vector<size_t> indices(allSpawnPoints.size());
            std::iota(indices.begin(), indices.end(), 0);

            std::random_device rd;
            std::mt19937 g(rd());
            std::shuffle(indices.begin(), indices.end(), g);

            // 유저 수만큼 랜덤 스폰 좌표 문자열 생성 (층 생략)
            std::string spawnStr;
            for (size_t i = 0; i < userCount; ++i) {
                const std::string& userId = room->users[i];  // id
                string nickname = Trim(GetNicknameById(userId));   // 닉네임 변환

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
                const std::string& userId = room->users[i];                 // ID
                std::string nickname = Trim(GetNicknameById(userId));             // 닉네임 변환

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
                std::string userId = Trim(GetIdByNickname(nickname));  // 닉네임 → ID 변환
                //string userId = Trim(rawuserId);
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
            nickname = Trim(nickname); // 혹시 공백 제거

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
        else if (message.rfind("PLACE_BALLOON|", 0) == 0)
        {
            // 메시지: PLACE_BALLOON|닉네임|x,z|타입
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

            // 좌표 보정 (선택사항)
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
            // 메시지: REMOVE_BALLOON|닉네임|x,z|타입
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

            const int range = 3;
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
            // 메시지: WATER_HIT|맞은닉네임|데미지
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