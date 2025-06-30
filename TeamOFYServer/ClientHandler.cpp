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

        std::cout << "[수신] 클라이언트 메시지 - IP: " << client->ip << ", 메시지: '" << message << "'" << std::endl;

        if (message.rfind("JOIN|", 0) == 0)
        {
            // JOIN 메시지 페이로드 얻기
            std::string payload = message.substr(5);

            // 직접 Split (',' 기준)
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

            // 직접 Trim (앞뒤 공백 제거)
            auto trim = [](const std::string& s) -> std::string {
                size_t st = 0, ed = s.size() - 1;
                while (st < s.size() && std::isspace(static_cast<unsigned char>(s[st]))) ++st;
                while (ed > st && std::isspace(static_cast<unsigned char>(s[ed]))) --ed;
                return s.substr(st, ed - st + 1);
                };

            if (tokens.size() != 2)
            {
                response = "ERROR|JOIN 메시지 형식 오류\n";
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
                    response = "ERROR|잘못된 모델 타입\n";
                }

                if (nickname.empty())
                    response = "ERROR|닉네임을 입력하세요\n";

                if (response.empty())
                {
                    client->id = nickname;
                    client->modelType = modelType;
                    std::cout << "JOIN - 닉네임: " << nickname << ", 모델타입: " << modelType << std::endl;
                    response = "JOIN_SUCCESS|\n";
                }
            }

            int sendLen = send(client->socket, response.c_str(), static_cast<int>(response.size()), 0);
            if (sendLen == SOCKET_ERROR)
            {
                std::cout << "[ProcessMessages] send 오류 - IP: " << client->ip << ", 포트: " << client->port
                    << ", 오류 코드: " << WSAGetLastError() << std::endl;
                break;
            }
            std::cout << "[ProcessMessages] 클라이언트 응답 전송 - IP: " << client->ip << ":" << client->port
                << " -> " << response;
        }

        if (pos == std::string::npos)
            break;

        start = pos + 1;
    }
}

