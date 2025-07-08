#include "UserManager.h"
#include "Server.h"

UserManager::UserManager(Server& server)
    : server_(server){}

vector<User> UserManager::LoadUsers(const string& filename) {
    vector<User> loadedUsers;
    ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "���� ���� ����: " << filename << std::endl;
        return loadedUsers;
    }

    string line;
    if (!getline(file, line)) {
        std::cerr << "��� �б� ����" << std::endl;
        return loadedUsers;
    }

    int count = 0;
    while (getline(file, line)) {
        if (line.empty()) continue; // �� �� �ǳʶٱ�

        stringstream ss(line);
        string id, pw, nick, levelStr, expStr;
        if (getline(ss, id, ',') && getline(ss, pw, ',') && getline(ss, nick, ',') &&
            getline(ss, levelStr, ',') && getline(ss, expStr)) {
            try {
                User user = { id, pw, nick, stoi(levelStr), stof(expStr) };
                loadedUsers.push_back(user);
                count++;
            }
            catch (const std::exception& e) {
                std::cerr << "���� ��ȯ ����: " << e.what() << " | ����: " << line << std::endl;
            }
        }
        else {
            std::cerr << "�Ľ� ���� ����: " << line << std::endl;
        }
    }

    std::cerr << "�ε�� ����� ��: " << count << std::endl;

    {
        lock_guard<mutex> lock(usersMutex);
        users = std::move(loadedUsers);
    }

    return users;
}

void UserManager::SaveUsers(const vector<User>& users, const string& filename) {
    ofstream file(filename);
    if (!file.is_open()) {
        cout << "[����] ����� ���� ���� ����" << endl;
        return;
    }
    file << "id,password,nickname,level,exp\n";
    for (const auto& user : users) {
        file << user.id << "," << user.password << "," << user.nickname << "," << user.level << "," << user.exp << "\n";
    }
}

string UserManager::CheckLogin(const string& id, const string& pw) {
    lock_guard<mutex> lock(usersMutex); // ���ο��� ���

    for (const auto& user : users) {
        if (user.id == id) {
            if (user.password == pw) {
                stringstream ss;
                ss << "LOGIN_SUCCESS|" << user.id << "," << user.password << ","
                    << user.nickname << "," << user.level << "," << user.exp;
                return ss.str() + "\n";
            }
            else {
                return "WRONG_PASSWORD|\n";
            }
        }
    }
    return "ID_NOT_FOUND|\n";
}

string UserManager::RegisterUser(const string& id, const string& pw, const string& nickname) {
    lock_guard<mutex> lock(usersMutex);

    for (const auto& user : users) {
        if (user.id == id)
            return "DUPLICATE_ID|\n";
        if (user.nickname == nickname)
            return "DUPLICATE_NICK|\n";
    }

    if (pw.empty())
        return "EMPTY_PASSWORD|\n";

    User newUser = { id, pw, nickname, 1, 0 };

    // ���Ͽ� ����
    ofstream outFile("users.csv", ios::app);
    if (!outFile.is_open())
        return "FILE_WRITE_ERROR|\n";

    outFile << newUser.id << "," << newUser.password << "," << newUser.nickname << "," << newUser.level << "," << newUser.exp << "\n";
    outFile.close();

    users.push_back(newUser);
    return "REGISTER_SUCCESS|\n";
}

void UserManager::BroadcastLobbyUserList() {
    std::string message = "LOBBY_USER_LIST";
    {
        lock_guard<mutex> lockUsers(usersMutex);
        auto& clients = server_.GetClients();

        for (const auto& c : clients) {
            if (c->id.empty()) continue;

            auto it = std::find_if(users.begin(), users.end(), [&](const User& u) {
                return u.id == c->id;
                });

            if (it != users.end()) {
                message += "|" + it->nickname + "," + std::to_string(it->level);
            }
            else {
                // ��ġ�ϴ� ���� ������ ������ �⺻�� level 1
                message += "|" + c->id + ",1";
            }
        }

        message += "\n";

        // ��� Ŭ���̾�Ʈ���� ����
        for (const auto& c : clients) {
            if (c->id.empty()) continue;
            int sendLen = send(c->socket, message.c_str(), (int)message.size(), 0);
            if (sendLen == SOCKET_ERROR) {
                cout << "[BroadcastLobbyUserList] send ���� - user: "
                    << c->id << ", ���� �ڵ�: " << WSAGetLastError() << endl;
            }
        }
    }

    cout << "[BroadcastLobbyUserList] �޽��� ���� �Ϸ�: " << message;
}

void UserManager::SendUserInfoByNickname(shared_ptr<ClientInfo> client, const string& nickname)
{
    lock_guard<mutex> lock(usersMutex);

    auto it = find_if(users.begin(), users.end(), [&](const User& u) {
        return u.nickname == nickname;
        });

    if (it != users.end())
    {
        const User& user = *it;

        // USER_INFO|nickname,level,exp\n
        string response = "USER_INFO|" + user.nickname + "," +
            to_string(user.level) + "," +
            to_string(user.exp) + "\n";

        send(client->socket, response.c_str(), (int)response.size(), 0);
    }
    else
    {
        string response = "USER_NOT_FOUND|\n";
        send(client->socket, response.c_str(), (int)response.size(), 0);
    }
}

void UserManager::BroadcastLobbyChatMessage(const string& nickname, const string& message) {
    string fullMsg = "LOBBY_CHAT|" + nickname + ":" + message + "\n";
    lock_guard<mutex> lock(server_.clientsMutex);
    for (const auto& client : server_.GetClients()) {
        send(client->socket, fullMsg.c_str(), (int)fullMsg.size(), 0);
    }
    cout << "[LobbyChat] " << nickname << ": " << message << endl;
}

void UserManager::LogoutUser(std::shared_ptr<ClientInfo> client) {
    std::lock_guard<std::mutex> lock(server_.GetClientsMutex());

    auto& clients = server_.GetClients();

    // clients ����Ʈ���� �ش� Ŭ���̾�Ʈ ���� �ʱ�ȭ
    for (auto& c : clients) {
        if (c->socket == client->socket) {
            c->id.clear();      // �г��� �ʱ�ȭ
            break;
        }
    }

    std::cout << "[UserManager] ���� �α׾ƿ� ó�� �Ϸ�: " << client->id << std::endl;
    BroadcastLobbyUserList();
}

