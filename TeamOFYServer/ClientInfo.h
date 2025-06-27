#pragma once
#include <iostream>
#include <winsock2.h>
#include <string>
#include "Utils.h"
class ClientInfo
{
public:
    SOCKET socket;
    std::string ip;
    int port;
    string id;

    ClientInfo();

    ClientInfo(SOCKET sock, const string& ip, int port)
        : socket(sock), ip(ip), port(port) {
    }

    ~ClientInfo() {
        if (socket != INVALID_SOCKET) {
            closesocket(socket);
            socket = INVALID_SOCKET;
        }
    }
};