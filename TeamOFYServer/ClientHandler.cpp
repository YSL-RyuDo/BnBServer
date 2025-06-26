#include "ClientHandler.h"

void ClientHandler::HandleClient(ClientInfo client) {
    char buffer[1025];

    cout << "[접속] 클라이언트 접속: " << client.ip << ":" << client.port << endl;

    while (true) {
        int recvLen = recv(client.socket, buffer, sizeof(buffer) - 1, 0);
        if (recvLen <= 0) break;

        buffer[recvLen] = '\0';
        string recvStr(buffer);

        //ProcessMessages(client, recvStr);
    }

    cout << "[접속 종료] 클라이언트 접속 종료: " << client.ip << ":" << client.port << endl;

    closesocket(client.socket);
    server_.RemoveClient(client);
}