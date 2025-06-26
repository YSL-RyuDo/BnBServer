#pragma once
#include <iostream>
#include <winsock2.h>
#include "Utils.h"
class ClientInfo
{
public:
    SOCKET socket;
    std::string ip;
    int port;
    string id;

    ClientInfo(SOCKET sock, const string& ip, int port)
        : socket(sock), ip(ip), port(port) {
    }
};