#include "UserManager.h"

vector<User> UserManager::LoadUsers(const string& filename) {
    vector<User> loadedUsers;
    ifstream file(filename);
    if (!file.is_open()) return loadedUsers;

    string line;
    getline(file, line); // header 무시

    while (getline(file, line)) {
        stringstream ss(line);
        string id, pw, nick, levelStr, expStr;
        if (getline(ss, id, ',') && getline(ss, pw, ',') && getline(ss, nick, ',') &&
            getline(ss, levelStr, ',') && getline(ss, expStr)) {
            User user = { id, pw, nick, stoi(levelStr), stof(expStr) };
            loadedUsers.push_back(user);
        }
    }
    return loadedUsers;
}

void UserManager::SaveUsers(const vector<User>& users, const string& filename) {
    ofstream file(filename);
    if (!file.is_open()) {
        cout << "[오류] 사용자 정보 저장 실패" << endl;
        return;
    }
    file << "id,password,nickname,level,exp\n";
    for (const auto& user : users) {
        file << user.id << "," << user.password << "," << user.nickname << "," << user.level << "," << user.exp << "\n";
    }
}