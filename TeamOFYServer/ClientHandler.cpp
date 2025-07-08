#include "ClientHandler.h"
#include "Server.h"

ClientHandler::ClientHandler(Server& server, UserManager& userManager, RoomManager& roomManager)
    : server_(server), userManager_(userManager), roomManager_(roomManager){}

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

        cout << "[수신] 클라이언트 메시지 - IP: " << client->ip << ", 메시지: '" << message << "'" << endl;
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
                }

                {
                    lock_guard<mutex> lock(server_.clientsMutex);

                    for (auto& c : server_.GetClients()) {
                        if (c->socket == client->socket) {
                            c->id = client->id;  // 덮어쓰기
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
        else if (message.rfind("CREATE_ROOM|", 0) == 0){
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