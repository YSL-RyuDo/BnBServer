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

        cout << "[수신] 클라이언트 메시지 - IP: " << client->ip << ", 메시지: '" << message << "'" << endl;
        string response;

        if (message.rfind("LOGIN|", 0) == 0) {
            cout << "로그인 요청 감지" << endl;
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
                            c->id = client->id;  // 덮어쓰기
                            break;
                        }
                    }

                    clientsMap[id] = client;  // nickname은 유일하므로 map은 그냥 덮어쓰기
                }
            }

            SendToClient(client, response);
        }
        else if (message.rfind("REGISTER|", 0) == 0) {
            cout << "[ProcessMessages] 회원가입 요청 감지" << endl;

            string data = message.substr(9);
            stringstream ss(data);
            string id, pw, nick;
            if (getline(ss, id, ',') && getline(ss, pw, ',') && getline(ss, nick)) {
                cout << "[ProcessMessages] 회원가입 데이터 파싱 완료 - id: " << id << ", pw: " << pw << ", nick: " << nick << endl;
                response = userManager_.RegisterUser(id, pw, nick);
            }
            else {
                response = "REGISTER_ERROR|\n";
                cout << "[ProcessMessages] 회원가입 데이터 형식 오류" << endl;
            }
            SendToClient(client, response);
        }
        else if (message == "QUIT|")
        {
            cout << "[ProcessMessages] 클라이언트 종료 요청 수신" << endl;
            closesocket(client->socket);
            server_.RemoveClient(client);
            return;
        }


        //else if (message.rfind("JOIN|", 0) == 0)
        //{
        //    // JOIN 메시지 페이로드 얻기
        //    std::string payload = message.substr(5);
        //    // 직접 Split (',' 기준)
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
        //    // 직접 Trim (앞뒤 공백 제거)
        //    auto trim = [](const std::string& s) -> std::string {
        //        size_t st = 0, ed = s.size() - 1;
        //        while (st < s.size() && std::isspace(static_cast<unsigned char>(s[st]))) ++st;
        //        while (ed > st && std::isspace(static_cast<unsigned char>(s[ed]))) --ed;
        //        return s.substr(st, ed - st + 1);
        //        };
        //    if (tokens.size() != 2)
        //    {
        //        response = "ERROR|JOIN 메시지 형식 오류\n";
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
        //            response = "ERROR|잘못된 모델 타입\n";
        //        }
        //        if (nickname.empty())
        //            response = "ERROR|닉네임을 입력하세요\n";
        //        if (response.empty())
        //        {
        //            client->id = nickname;
        //            client->modelType = modelType;
        //            std::cout << "JOIN - 닉네임: " << nickname << ", 모델타입: " << modelType << std::endl;
        //            response = "JOIN_SUCCESS|\n";
        //        }
        //    }
        //    if (!SendToClient(client, response))
        //        break;
        //}
        //else if (message.rfind("REQUEST_MODEL|", 0) == 0)
        //{
        //    std::string nickname = message.substr(strlen("REQUEST_MODEL|"));
        //    // 공백 제거 (선택)
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
        //        response = "ERROR|닉네임이 없습니다\n";
        //    }
        //    else
        //    {
        //        // 닉네임이 현재 접속 중인 클라이언트와 일치하는지 확인
        //        if (client->id == nickname)
        //        {
        //            response = "MODEL_TYPE|" + std::to_string(client->modelType) + "\n";
        //        }
        //        else
        //        {
        //            response = "ERROR|닉네임 불일치 또는 존재하지 않음\n";
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
        std::cout << "[SendToClient] send 오류 - IP: " << client->ip << ", 포트: " << client->port
            << ", 오류 코드: " << WSAGetLastError() << std::endl;
        return false;
    }

    std::cout << "[SendToClient] 클라이언트 응답 전송 - IP: " << client->ip << ":" << client->port
        << " -> " << response;
    return true;
}