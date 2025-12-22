#include "ClientHandler.h"
#include "Server.h"

ClientHandler::ClientHandler(Server& server, UserManager& userManager, RoomManager& roomManager, MapManager& mapManager, Player& player)
    : server_(server), userManager_(userManager), roomManager_(roomManager), mapManager_(mapManager), player_(player){}

ClientHandler::~ClientHandler() {}

void ClientHandler::HandleClient(shared_ptr<ClientInfo> client)
{
    char buffer[1025];

    while (true)
    {
        int recvLen = recv(client->socket, buffer, sizeof(buffer) - 1, 0);
        if (recvLen <= 0)
        {
            // recv == 0 또는 오류 발생
            break;
        }

        buffer[recvLen] = '\0';
        string recvStr(buffer);
        ProcessMessages(client, recvStr);
    }

    std::cout << "[DISCONNECT] client socket closed\n";

    // 1. 로그인 상태면 방 정리 + 로그아웃
    if (!client->id.empty())
    {
        // 방에 있으면 강제 퇴장 + 방 비면 삭제
        roomManager_.ForceExitRoomByUserId(client->id);

        // 로그아웃 처리 + 로비 유저 리스트 갱신
        userManager_.LogoutUser(client);
    }

    // 2. 서버 클라이언트 목록에서 제거
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

        else if (message.rfind("GETINFO|", 0) == 0)
        {
            std::string nickname = Trim(message.substr(strlen("GETINFO|")));
            std::string userId = Trim(GetIdByNickname(nickname));
            if (userId.empty())
            {
                std::cerr << "[GETINFO] 존재하지 않는 닉네임: " << nickname << std::endl;
                return;
            }

            try
            {
                UserProfile& profile = userManager_.GetUserProfileById(userId);
                UserWinLossStats& stats = userManager_.GetUserWinLossStatsById(userId);
                UserCharacterEmotes& emotes = userManager_.GetUserEmotesById(userId);
                UserBallons& ballons = userManager_.GetUserBallonsById(userId);
                UserIcons& icons = userManager_.GetUserIconsById(userId);

                // 요청자(client)한테 조회 결과 전송
                if (client)
                {
                    SendSetInfo(client, nickname, profile);
                    SendWinRate(client, nickname, stats);
                    SendUserEmotes(client, nickname);
                    SendUserBallons(client, nickname);
                    SendUserIcons(client, nickname);
                }
            }
            catch (const std::exception& e)
            {
                std::cerr << "[GETINFO] 처리 실패: " << e.what() << std::endl;
            }
        }
        else if (message.rfind("GET_BALLOON_INFO|", 0) == 0)
        {
            std::string nickname = Trim(message.substr(strlen("GET_BALLOON_INFO|")));
            std::string userId = Trim(GetIdByNickname(nickname));
            if (userId.empty())
            {
                std::cerr << "[GETINFO] 존재하지 않는 닉네임: " << nickname << std::endl;
                return;
            }

            try
            {
                UserBallons& ballons = userManager_.GetUserBallonsById(userId);

                // 요청자(client)한테 조회 결과 전송
                if (client)
                {
                    SendUserBallons(client, nickname);
                }
            }
            catch (const std::exception& e)
            {
                std::cerr << "[GETINFO] 처리 실패: " << e.what() << std::endl;
            }
        }
        else if (message.rfind("GET_EMO_INFO|", 0) == 0)
        {
            std::string nickname = Trim(message.substr(strlen("GET_EMO_INFO|")));
            std::string userId = Trim(GetIdByNickname(nickname));
            if (userId.empty())
            {
                std::cerr << "[GETINFO] 존재하지 않는 닉네임: " << nickname << std::endl;
                return;
            }

            try
            {
                UserCharacterEmotes& emotes = userManager_.GetUserEmotesById(userId);

                // 요청자(client)한테 조회 결과 전송
                if (client)
                {
                    SendUserEmotes(client, nickname);
                }
            }
            catch (const std::exception& e)
            {
                std::cerr << "[GETINFO] 처리 실패: " << e.what() << std::endl;
            }
        }
        else if (message.rfind("GET_ICON_INFO|", 0) == 0)
        {
            std::string nickname = Trim(message.substr(strlen("GET_ICON_INFO|")));
            std::string userId = Trim(GetIdByNickname(nickname));
            if (userId.empty())
            {
                std::cerr << "[GET_ICON_INFO] 존재하지 않는 닉네임: " << nickname << std::endl;
                return;
            }

            try
            {
                if (client)
                {
                    SendUserIcons(client, nickname);
                }
            }
            catch (const std::exception& e)
            {
                std::cerr << "[GET_ICON_INFO] 처리 실패: " << e.what() << std::endl;
            }
            }
        else if (message.rfind("EMO_CHANGE|", 0) == 0)
        {
            std::string data = message.substr(strlen("EMO_CHANGE|"));
            std::stringstream ss(data);

            std::string nickname;
            std::string aStr, bStr;

            if (!std::getline(ss, nickname, ',') ||
                !std::getline(ss, aStr, ',') ||
                !std::getline(ss, bStr))
                return;

            int emoA = std::stoi(aStr); // 변경당하는 (슬롯에 있어야 함)
            int emoB = std::stoi(bStr); // 변경하려는

            if (emoA == emoB)
                return;

            std::string userId = Trim(GetIdByNickname(nickname));
            if (userId.empty())
                return;

            UserProfile& profile = userManager_.GetUserProfileById(userId);
            UserCharacterEmotes& emotes = userManager_.GetUserEmotesById(userId);

            // 1️ B 보유 여부 검사
            auto hasEmo = [&](int emo) -> bool {
                switch (emo)
                {
                case 0: return emotes.emo0;
                case 1: return emotes.emo1;
                case 2: return emotes.emo2;
                case 3: return emotes.emo3;
                case 4: return emotes.emo4;
                case 5: return emotes.emo5;
                case 6: return emotes.emo6;
                case 7: return emotes.emo7;
                case 8: return emotes.emo8;
                case 9: return emotes.emo9;
                case 10: return emotes.emo10;
                case 11: return emotes.emo11;
                case 12: return emotes.emo12;
                case 13: return emotes.emo13;
                case 14: return emotes.emo14;
                case 15: return emotes.emo15;
                case 16: return emotes.emo16;
                case 17: return emotes.emo17;
                case 18: return emotes.emo18;
                case 19: return emotes.emo19;
                case 20: return emotes.emo20;
                case 21: return emotes.emo21;
                case 22: return emotes.emo22;
                case 23: return emotes.emo23;
                case 24: return emotes.emo24;
                case 25: return emotes.emo25;
                case 26: return emotes.emo26;
                case 27: return emotes.emo27;
                case 28: return emotes.emo28;
                case 29: return emotes.emo29;
                case 30: return emotes.emo30;
                case 31: return emotes.emo31;
                case 32: return emotes.emo32;
                case 33: return emotes.emo33;
                case 34: return emotes.emo34;
                case 35: return emotes.emo35;
                default: return false;
                }
                };

            if (!hasEmo(emoB))
            {
                send(client->socket,
                    "EMO_CHANGE_FAIL|NOT_OWNED\n",
                    26, 0);
                return;
            }

            // 2️ 슬롯 찾기
            int* slots[4] = {
                &profile.emo0,
                &profile.emo1,
                &profile.emo2,
                &profile.emo3
            };

            int idxA = -1;
            int idxB = -1;

            for (int i = 0; i < 4; ++i)
            {
                if (*slots[i] == emoA) idxA = i;
                if (*slots[i] == emoB) idxB = i;
            }

            // A는 반드시 슬롯에 있어야 함
            if (idxA == -1)
            {
                send(client->socket,
                    "EMO_CHANGE_FAIL|NOT_EQUIPPED\n",
                    30, 0);
                return;
            }

            // 3️ 처리
            if (idxB != -1)
            {
                // 슬롯 (스왑)
                std::swap(*slots[idxA], *slots[idxB]);
            }
            else
            {
                // 슬롯 (교체)
                *slots[idxA] = emoB;
            }

            userManager_.SaveUserProfile(userId);

            SendSetInfo(client, nickname, profile);

            std::cout << "[EMO_CHANGE] 성공: "
                << nickname << " "
                << emoA << " -> " << emoB << std::endl;
        }
        else if (message.rfind("BALLOON_CHANGE|", 0) == 0)
        {
            std::string data = message.substr(strlen("BALLOON_CHANGE|"));
            std::stringstream ss(data);

            std::string nickname;
            std::string aStr, bStr;

            if (!std::getline(ss, nickname, ',') ||
                !std::getline(ss, aStr, ',') ||
                !std::getline(ss, bStr))
                return;

            int fromBalloon = std::stoi(aStr); // 변경당하는 (현재 장착)
            int toBalloon = std::stoi(bStr); // 변경하려는

            if (fromBalloon == toBalloon)
                return;

            std::string userId = Trim(GetIdByNickname(nickname));
            if (userId.empty())
                return;

            UserProfile& profile = userManager_.GetUserProfileById(userId);
            UserBallons& ballons = userManager_.GetUserBallonsById(userId);

            // 1️ 장착 중인지 확인
            if (profile.balloon != fromBalloon)
            {
                send(client->socket,
                    "BALLOON_CHANGE_FAIL|NOT_EQUIPPED\n",
                    35, 0);
                return;
            }

            // 2️ 보유 여부 확인
            int values[10] = {
                ballons.balloon0, ballons.balloon1, ballons.balloon2,
                ballons.balloon3, ballons.balloon4, ballons.balloon5,
                ballons.balloon6, ballons.balloon7, ballons.balloon8,
                ballons.balloon9
            };

            if (toBalloon < 0 || toBalloon >= 10 || values[toBalloon] == 0)
            {
                send(client->socket,
                    "BALLOON_CHANGE_FAIL|NOT_OWNED\n",
                    33, 0);
                return;
            }

            // 3️ 교체
            profile.balloon = toBalloon;
            userManager_.SaveUserProfile(userId);

            // 4️ 결과 전송
            SendSetInfo(client, nickname, profile);

            std::cout << "[BALLOON_CHANGE] 성공: "
                << nickname << " "
                << fromBalloon << " -> " << toBalloon << std::endl;
        }
        else if (message.rfind("ICON_CHANGE|", 0) == 0)
        {
            std::string data = message.substr(strlen("ICON_CHANGE|"));
            std::stringstream ss(data);

            std::string nickname;
            std::string aStr, bStr;

            if (!std::getline(ss, nickname, ',') ||
                !std::getline(ss, aStr, ',') ||
                !std::getline(ss, bStr))
                return;

            int fromIcon = std::stoi(aStr); // 변경당하는
            int toIcon = std::stoi(bStr); // 변경하려는

            if (fromIcon == toIcon)
                return;

            std::string userId = Trim(GetIdByNickname(nickname));
            if (userId.empty())
                return;

            UserProfile& profile = userManager_.GetUserProfileById(userId);
            UserIcons& icons = userManager_.GetUserIconsById(userId);

            // 1️ 장착 중인지 확인
            if (profile.icon != fromIcon)
            {
                send(client->socket,
                    "ICON_CHANGE_FAIL|NOT_EQUIPPED\n",
                    32, 0);
                return;
            }

            // 2️ 보유 여부 확인
            int values[10] = {
                icons.icon0, icons.icon1, icons.icon2, icons.icon3,
                icons.icon4, icons.icon5, icons.icon6, icons.icon7,
                icons.icon8, icons.icon9
            };

            if (toIcon < 0 || toIcon >= 10 || values[toIcon] == 0)
            {
                send(client->socket,
                    "ICON_CHANGE_FAIL|NOT_OWNED\n",
                    30, 0);
                return;
            }

            // 3️ 교체
            profile.icon = toIcon;
            userManager_.SaveUserProfile(userId);

            // 4️ 결과 전송
            SendSetInfo(client, nickname, profile);

            std::cout << "[ICON_CHANGE] 성공: "
                << nickname << " "
                << fromIcon << " -> " << toIcon << std::endl;
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

// SETINFO 패킷 전송 함수
void ClientHandler::SendSetInfo(std::shared_ptr<ClientInfo> client, const std::string& nickname, const UserProfile& profile)
{
    std::stringstream ss;
    ss << "SETINFO|"
        << nickname << ","
        << profile.level << ","
        << profile.exp << ","
        << profile.icon << ","
        << profile.emo0 << ","
        << profile.emo1 << ","
        << profile.emo2 << ","
        << profile.emo3 << ","
        << profile.balloon;

    std::string msg = ss.str() + "\n";
    SendToClient(client, msg);
    std::cout << "[SETINFO] 전송: " << msg;
}

// WINRATE 패킷 전송 함수
void ClientHandler::SendWinRate(std::shared_ptr<ClientInfo> client, const std::string& nickname, const UserWinLossStats& stats)
{
    struct CharWinLose { int index; int win; int lose; int total; };
    std::vector<CharWinLose> charStats;
    for (int i = 0; i <= 6; ++i)
    {
        int win = 0, lose = 0;
        switch (i)
        {
        case 0: win = stats.char0_win; lose = stats.char0_lose; break;
        case 1: win = stats.char1_win; lose = stats.char1_lose; break;
        case 2: win = stats.char2_win; lose = stats.char2_lose; break;
        case 3: win = stats.char3_win; lose = stats.char3_lose; break;
        case 4: win = stats.char4_win; lose = stats.char4_lose; break;
        case 5: win = stats.char5_win; lose = stats.char5_lose; break;
        case 6: win = stats.char6_win; lose = stats.char6_lose; break;
        }
        charStats.push_back({ i, win, lose, win + lose });
    }

    std::sort(charStats.begin(), charStats.end(),
        [](const CharWinLose& a, const CharWinLose& b) { return a.total > b.total; });

    std::stringstream ss;
    ss << "WINRATE|"
        << nickname << "," << stats.winCount << "," << stats.loseCount;

    for (int i = 0; i < 3; ++i)
    {
        ss << "," << charStats[i].index
            << "," << charStats[i].win
            << "," << charStats[i].lose;
    }

    std::string msg = ss.str() + "\n";
    SendToClient(client, msg);
    std::cout << "[WINRATE] 전송: " << msg;
}

// 클라이언트에게 GETMYEMO 패킷 전송
void ClientHandler::SendUserEmotes(std::shared_ptr<ClientInfo> client, const std::string& nickname)
{
    std::string userId = Trim(GetIdByNickname(nickname));
    if (userId.empty())
    {
        std::cerr << "[GETMYEMO] 존재하지 않는 닉네임: " << nickname << std::endl;
        return;
    }

    UserCharacterEmotes& emotes = userManager_.GetUserEmotesById(userId);

    std::stringstream ss;
    ss << "GETMYEMO|" << nickname;

    int emoteArray[36] = {
        emotes.emo0, emotes.emo1, emotes.emo2, emotes.emo3,
        emotes.emo4, emotes.emo5, emotes.emo6, emotes.emo7,
        emotes.emo8, emotes.emo9, emotes.emo10, emotes.emo11,
        emotes.emo12, emotes.emo13, emotes.emo14, emotes.emo15,
        emotes.emo16, emotes.emo17, emotes.emo18, emotes.emo19,
        emotes.emo20, emotes.emo21, emotes.emo22, emotes.emo23,
        emotes.emo24, emotes.emo25, emotes.emo26, emotes.emo27,
        emotes.emo28, emotes.emo29, emotes.emo30, emotes.emo31,
        emotes.emo32, emotes.emo33, emotes.emo34, emotes.emo35
    };

    // 1인 인덱스만 전송
    for (int i = 0; i < 36; ++i) {
        if (emoteArray[i] == 1)
            ss << "," << i;
    }

    std::string msg = ss.str() + "\n";
    SendToClient(client, msg);
    std::cout << "[GETMYEMO] 전송: " << msg;
}

// 클라이언트에게 GETMYBALLOON 패킷 전송
void ClientHandler::SendUserBallons(std::shared_ptr<ClientInfo> client, const std::string& nickname)
{
    // 닉네임 → ID
    std::string userId = Trim(GetIdByNickname(nickname));
    if (userId.empty())
    {
        std::cerr << "[GETMYBALLOON] 존재하지 않는 닉네임: " << nickname << std::endl;
        return;
    }

    // UserManager에서 해당 ID의 물풍선 데이터 가져오기 (참조 반환)
    UserBallons& ballons = userManager_.GetUserBallonsById(userId);

    std::stringstream ss;
    ss << "GETMYBALLOON|" << nickname;

    int ballonArray[10] = {
        ballons.balloon0, ballons.balloon1, ballons.balloon2, ballons.balloon3,
        ballons.balloon4, ballons.balloon5, ballons.balloon6, ballons.balloon7,
        ballons.balloon8, ballons.balloon9
    };

    // 값이 1인 인덱스만 추가
    for (int i = 0; i < 10; ++i)
    {
        if (ballonArray[i] == 1) // 1 = 보유
            ss << "," << i;      // 인덱스만 전송
    }

    std::string msg = ss.str() + "\n";
    SendToClient(client, msg);
    std::cout << "[GETMYBALLOON] 전송: " << msg;
}

void ClientHandler::SendUserIcons(
    std::shared_ptr<ClientInfo> client,
    const std::string& nickname)
{
    std::string userId = Trim(GetIdByNickname(nickname));
    if (userId.empty() || !client)
        return;

    UserProfile& profile = userManager_.GetUserProfileById(userId);
    UserIcons& icons = userManager_.GetUserIconsById(userId);

    std::stringstream packet;
    std::stringstream log;

    // ===== 패킷 시작 =====
    packet << "GETMYICON|" << nickname;

    // ===== 콘솔 로그 =====
    log << "[GETMYICON] 닉네임=" << nickname
        << " 장착아이콘=" << profile.icon
        << " 보유 아이콘: ";

    // 보유 여부 배열
    int values[10] = {
        icons.icon0, icons.icon1, icons.icon2, icons.icon3,
        icons.icon4, icons.icon5, icons.icon6, icons.icon7,
        icons.icon8, icons.icon9
    };

    bool first = true;
    for (int i = 0; i < 10; ++i)
    {
        if (values[i])
        {
            packet << "," << i;

            if (!first) log << ",";
            log << i;
            first = false;
        }
    }

    packet << "\n";

    // ===== 전송 =====
    send(client->socket,
        packet.str().c_str(),
        (int)packet.str().size(),
        0);

    // ===== 콘솔 출력 =====
    std::cout << log.str() << std::endl;
}

// ClientHandler::HandleClient 종료 시
void ClientHandler::OnClientDisconnected(shared_ptr<ClientInfo> client)
{
    if (!client) return;
    if (!client->id.empty())
    {
        // 방 검사
        Room* room = roomManager_.GetRoomByUserId(client->id);

        if (room)
        {
            // 방에서 강제 제거
            roomManager_.ForceExitRoomByUserId(client->id);
        }

        // 로그아웃
        userManager_.LogoutUser(client);
    }

    // 서버에서 클라이언트 제거
    server_.RemoveClient(client);
}

void ClientHandler::RemoveLoginSession(const std::string& userId)
{
    // 서버의 기존 clientsMutex 사용
    std::lock_guard<std::mutex> lock(server_.clientsMutex);

    auto it = clientsMap.find(userId);
    if (it != clientsMap.end())
    {
        clientsMap.erase(it);
        std::cout << "[ClientHandler] 로그인 세션 제거: " << userId << std::endl;
    }
}



