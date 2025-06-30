#include "ClientHandler.h"
#include "Server.h"

ClientHandler::ClientHandler(Server& server) : server_(server) {}

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

        std::string message;
        if (pos == std::string::npos)
            message = recvStr.substr(start);
        else
            message = recvStr.substr(start, pos - start);

        if (message.empty())
            break;

        std::cout << "[����] Ŭ���̾�Ʈ �޽��� - IP: " << client->ip << ", �޽���: '" << message << "'" << std::endl;

        if (message.rfind("JOIN|", 0) == 0)
        {
            // JOIN �޽��� ���̷ε� ���
            std::string payload = message.substr(5);

            // ���� Split (',' ����)
            std::vector<std::string> tokens;
            size_t splitStart = 0;
            while (true)
            {
                size_t commaPos = payload.find(',', splitStart);
                if (commaPos == std::string::npos)
                {
                    tokens.push_back(payload.substr(splitStart));
                    break;
                }
                tokens.push_back(payload.substr(splitStart, commaPos - splitStart));
                splitStart = commaPos + 1;
            }

            std::string response;

            // ���� Trim (�յ� ���� ����)
            auto trim = [](const std::string& s) -> std::string {
                size_t st = 0, ed = s.size() - 1;
                while (st < s.size() && std::isspace(static_cast<unsigned char>(s[st]))) ++st;
                while (ed > st && std::isspace(static_cast<unsigned char>(s[ed]))) --ed;
                return s.substr(st, ed - st + 1);
                };

            if (tokens.size() != 2)
            {
                response = "ERROR|JOIN �޽��� ���� ����\n";
            }
            else
            {
                std::string nickname = trim(tokens[0]);
                int modelType = 0;
                try
                {
                    modelType = std::stoi(tokens[1]);
                }
                catch (...)
                {
                    response = "ERROR|�߸��� �� Ÿ��\n";
                }

                if (nickname.empty())
                    response = "ERROR|�г����� �Է��ϼ���\n";

                if (response.empty())
                {
                    client->id = nickname;
                    client->modelType = modelType;
                    std::cout << "JOIN - �г���: " << nickname << ", ��Ÿ��: " << modelType << std::endl;
                    response = "JOIN_SUCCESS|\n";
                }
            }

            int sendLen = send(client->socket, response.c_str(), static_cast<int>(response.size()), 0);
            if (sendLen == SOCKET_ERROR)
            {
                std::cout << "[ProcessMessages] send ���� - IP: " << client->ip << ", ��Ʈ: " << client->port
                    << ", ���� �ڵ�: " << WSAGetLastError() << std::endl;
                break;
            }
            std::cout << "[ProcessMessages] Ŭ���̾�Ʈ ���� ���� - IP: " << client->ip << ":" << client->port
                << " -> " << response;
        }

        if (pos == std::string::npos)
            break;

        start = pos + 1;
    }
}

