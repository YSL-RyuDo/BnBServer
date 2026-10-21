#pragma once
#include <iostream>
#include <winsock2.h>
#include <string>
#include "Utils.h"
class ClientInfo
{
public:
    SOCKET socket;
    string ip;
    int port;
    string id;
    string nickname;
    int modelType = -1;
    ClientInfo();

    ClientInfo(SOCKET sock, const string& ip, int port)
        : socket(sock), ip(ip), port(port), output_(SocketOutput::Create(sock)) {
    }

    void Disconnect() { if (output_) output_->Stop(); }
    bool IsClosing() const { return output_ && output_->IsStopping(); }
    ~ClientInfo() = default; // SocketOutput joins its writer and closes the socket once.
private:
    std::shared_ptr<SocketOutput> output_;
};
