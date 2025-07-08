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

        cout << "[����] Ŭ���̾�Ʈ �޽��� - IP: " << client->ip << ", �޽���: '" << message << "'" << endl;
        string response;

        if (message.rfind("LOGIN|", 0) == 0) {
            cout << "�α��� ��û ����" << endl;
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
                            c->id = client->id;  // �����
                            break;
                        }
                    }

                    clientsMap[id] = client;  // nickname�� �����ϹǷ� map�� �׳� �����
                }
            }

            SendToClient(client, response);
        }
        else if (message.rfind("REGISTER|", 0) == 0) {
            cout << "[ProcessMessages] ȸ������ ��û ����" << endl;

            string data = message.substr(strlen("REGISTER|"));
            stringstream ss(data);
            string id, pw, nick;
            if (getline(ss, id, ',') && getline(ss, pw, ',') && getline(ss, nick)) {
                cout << "[ProcessMessages] ȸ������ ������ �Ľ� �Ϸ� - id: " << id << ", pw: " << pw << ", nick: " << nick << endl;
                response = userManager_.RegisterUser(id, pw, nick);
            }
            else {
                response = "REGISTER_ERROR|\n";
                cout << "[ProcessMessages] ȸ������ ������ ���� ����" << endl;
            }
            SendToClient(client, response);
        }
        else if (message == "QUIT|")
        {
            cout << "[ProcessMessages] Ŭ���̾�Ʈ ���� ��û ����" << endl;
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
            cout << "���� ��������Ʈ ��û ����" << endl;
            userManager_.BroadcastLobbyUserList();
        }
        else if (message == "GET_ROOM_LIST|")
        {
            cout << "�� ���� ����Ʈ ��û ����" << endl;
            roomManager_.BroadcastRoomlist(client);
        }
        else if (message.rfind("LOBBY_MESSAGE|", 0) == 0)
        {
            cout << "[ProcessMessages] �κ� �޽��� ����" << endl;

            string data = message.substr(strlen("LOBBY_MESSAGE|")); // "�г���:�޽���"
            size_t delimPos = data.find(':');

            if (delimPos != string::npos) {
                string nickname = data.substr(0, delimPos);
                string chatMsg = data.substr(delimPos + 1);

                userManager_.BroadcastLobbyChatMessage(nickname, chatMsg);
                response.clear();
            }
            else {
                cerr << "[ProcessMessages] ä�� �޽��� ���� ����: " << data << endl;
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

                // �� �Ŵ����� ����
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
            // ���� ó�� �ڵ带 RoomManager�� �ѱ��
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
        std::cout << "[SendToClient] send ���� - IP: " << client->ip << ", ��Ʈ: " << client->port
            << ", ���� �ڵ�: " << WSAGetLastError() << std::endl;
        return false;
    }

    std::cout << "[SendToClient] Ŭ���̾�Ʈ ���� ���� - IP: " << client->ip << ":" << client->port
        << " -> " << response;
    return true;
}