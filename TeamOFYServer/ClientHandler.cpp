#include "ClientHandler.h"

void ClientHandler::HandleClient(ClientInfo client) {
    char buffer[1025];

    cout << "[����] Ŭ���̾�Ʈ ����: " << client.ip << ":" << client.port << endl;

    while (true) {
        int recvLen = recv(client.socket, buffer, sizeof(buffer) - 1, 0);
        if (recvLen <= 0) break;

        buffer[recvLen] = '\0';
        string recvStr(buffer);

        //ProcessMessages(client, recvStr);
    }

    cout << "[���� ����] Ŭ���̾�Ʈ ���� ����: " << client.ip << ":" << client.port << endl;

    closesocket(client.socket);
    server_.RemoveClient(client);
}