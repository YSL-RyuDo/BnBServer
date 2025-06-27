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
        std::string recvStr(buffer);
        ProcessMessages(client, recvStr);
    };
    closesocket(client->socket);
    server_.RemoveClient(client);
}

void ClientHandler::ProcessMessages(std::shared_ptr<ClientInfo> client, const std::string& recvStr) {
    size_t start = 0;
    while (true) {
        size_t pos = recvStr.find('\n', start);
        std::string message;
        if (pos == std::string::npos) {
            message = recvStr.substr(start);
        }
        else {
            message = recvStr.substr(start, pos - start);
        }

        if (!message.empty()) {
            std::cout << "[수신] 클라이언트 메시지 - IP: " << client->ip << ", 메시지: '" << message << "'" << std::endl;

            std::string response;

            response = "서버 응답: " + message;

            if (!response.empty()) {
                response;
                int sendLen = send(client->socket, response.c_str(), static_cast<int>(response.size()), 0);
                if (sendLen == SOCKET_ERROR) {
                    std::cout << "[ProcessMessages] send 오류 - IP: " << client->ip << ", 포트: " << client->port
                        << ", 오류 코드: " << WSAGetLastError() << std::endl;
                    break;
                }
                std::cout << "[ProcessMessages] 클라이언트 응답 전송 - IP: " << client->ip << ":" << client->port
                    << " -> " << response << std::endl;
            }
        }

        if (pos == std::string::npos) break;
        start = pos + 1;
    }
}
