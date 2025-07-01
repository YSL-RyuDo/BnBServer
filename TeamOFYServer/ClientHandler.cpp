#include "ClientHandler.h"
#include "Server.h"

ClientHandler::ClientHandler(Server& server, UserManager& userManager)
    : server_(server), userManager_(userManager) {}

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
                size_t colonPos = response.find(':');
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

                    for (auto& c : clients) {
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

            string data = message.substr(9);
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


        //else if (message.rfind("JOIN|", 0) == 0)
        //{
        //    // JOIN �޽��� ���̷ε� ���
        //    std::string payload = message.substr(5);
        //    // ���� Split (',' ����)
        //    std::vector<std::string> tokens;
        //    size_t splitStart = 0;
        //    while (true)
        //    {
        //        size_t commaPos = payload.find(',', splitStart);
        //        if (commaPos == std::string::npos)
        //        {
        //            tokens.push_back(payload.substr(splitStart));
        //            break;
        //        }
        //        tokens.push_back(payload.substr(splitStart, commaPos - splitStart));
        //        splitStart = commaPos + 1;
        //    }
        //    std::string response;
        //    // ���� Trim (�յ� ���� ����)
        //    auto trim = [](const std::string& s) -> std::string {
        //        size_t st = 0, ed = s.size() - 1;
        //        while (st < s.size() && std::isspace(static_cast<unsigned char>(s[st]))) ++st;
        //        while (ed > st && std::isspace(static_cast<unsigned char>(s[ed]))) --ed;
        //        return s.substr(st, ed - st + 1);
        //        };
        //    if (tokens.size() != 2)
        //    {
        //        response = "ERROR|JOIN �޽��� ���� ����\n";
        //    }
        //    else
        //    {
        //        std::string nickname = trim(tokens[0]);
        //        int modelType = 0;
        //        try
        //        {
        //            modelType = std::stoi(tokens[1]);
        //        }
        //        catch (...)
        //        {
        //            response = "ERROR|�߸��� �� Ÿ��\n";
        //        }
        //        if (nickname.empty())
        //            response = "ERROR|�г����� �Է��ϼ���\n";
        //        if (response.empty())
        //        {
        //            client->id = nickname;
        //            client->modelType = modelType;
        //            std::cout << "JOIN - �г���: " << nickname << ", ��Ÿ��: " << modelType << std::endl;
        //            response = "JOIN_SUCCESS|\n";
        //        }
        //    }
        //    if (!SendToClient(client, response))
        //        break;
        //}
        //else if (message.rfind("REQUEST_MODEL|", 0) == 0)
        //{
        //    std::string nickname = message.substr(strlen("REQUEST_MODEL|"));
        //    // ���� ���� (����)
        //    auto trim = [](const std::string& s) -> std::string {
        //        size_t st = 0, ed = s.size() - 1;
        //        while (st < s.size() && std::isspace(static_cast<unsigned char>(s[st]))) ++st;
        //        while (ed > st && std::isspace(static_cast<unsigned char>(s[ed]))) --ed;
        //        return s.substr(st, ed - st + 1);
        //        };
        //    nickname = trim(nickname);
        //    std::string response;
        //    if (nickname.empty())
        //    {
        //        response = "ERROR|�г����� �����ϴ�\n";
        //    }
        //    else
        //    {
        //        // �г����� ���� ���� ���� Ŭ���̾�Ʈ�� ��ġ�ϴ��� Ȯ��
        //        if (client->id == nickname)
        //        {
        //            response = "MODEL_TYPE|" + std::to_string(client->modelType) + "\n";
        //        }
        //        else
        //        {
        //            response = "ERROR|�г��� ����ġ �Ǵ� �������� ����\n";
        //        }
        //    }
        //    if (!SendToClient(client, response))
        //        break;
        //}

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