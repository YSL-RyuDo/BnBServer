#include "UserManager.h"

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
