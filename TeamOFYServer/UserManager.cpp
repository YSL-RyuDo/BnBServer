#include "UserManager.h"
#include "Server.h"

UserManager::UserManager(Server& server)
    : server_(server){}

vector<UserAccount> UserManager::LoadAccountUsers(const string& filename) {
    vector<UserAccount> loadedUsers;
    ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "파일 열기 실패: " << filename << std::endl;
        return loadedUsers;
    }

    string line;
    if (!getline(file, line)) {
        std::cerr << "헤더 읽기 실패" << std::endl;
        return loadedUsers;
    }

    int count = 0;
    while (getline(file, line)) {
        if (line.empty()) continue; // 빈 줄 건너뛰기

        stringstream ss(line);
        string id, pw, nick, levelStr, expStr;
        if (getline(ss, id, ',') && getline(ss, pw, ',') && getline(ss, nick, ',')) {
            try {
                UserAccount userAccount = { id, pw, nick};
                loadedUsers.push_back(userAccount);
                count++;
            }
            catch (const std::exception& e) {
                std::cerr << "숫자 변환 오류: " << e.what() << " | 라인: " << line << std::endl;
            }
        }
        else {
            std::cerr << "파싱 실패 라인: " << line << std::endl;
        }
    }

    std::cerr << "로드된 사용자 수: " << count << std::endl;

    {
        lock_guard<mutex> lock(usersMutex);
        users = std::move(loadedUsers);
    }

    return users;
}

std::vector<UserProfile> UserManager::LoadUserProfiles(const std::string& filename)
{
    std::ifstream file(filename);
    std::vector<UserProfile> loadedProfiles;

    if (!file.is_open()) {
        std::cerr << "파일 열기 실패: " << filename << std::endl;
        return loadedProfiles;
    }

    std::string line;
    if (!std::getline(file, line)) {
        std::cerr << "헤더 읽기 실패: " << filename << std::endl;
        return loadedProfiles;
    }

    while (std::getline(file, line)) {
        if (line.empty()) continue;

        std::stringstream ss(line);
        UserProfile profile;
        std::string expStr;

        if (std::getline(ss, profile.id, ',') &&
            std::getline(ss, line, ',') && (profile.level = std::stoi(line), true) &&
            std::getline(ss, expStr, ',') && (profile.exp = std::stof(expStr), true) &&
            std::getline(ss, line, ',') && (profile.icon = std::stoi(line), true) &&
            std::getline(ss, line, ',') && (profile.money0 = std::stoi(line), true) &&
            std::getline(ss, line, ',') && (profile.money1 = std::stoi(line), true) &&
            std::getline(ss, line, ',') && (profile.emo0 = std::stoi(line), true) &&
            std::getline(ss, line, ',') && (profile.emo1 = std::stoi(line), true) &&
            std::getline(ss, line, ',') && (profile.emo2 = std::stoi(line), true) &&
            std::getline(ss, line, ',') && (profile.emo3 = std::stoi(line), true) &&
            std::getline(ss, line) && (profile.balloon = std::stoi(line), true)
            )
        {
            loadedProfiles.push_back(profile);
        }
        else {
            std::cerr << "UserProfile.csv 파싱 오류 라인: " << line << std::endl;
        }
    }

    userProfiles = std::move(loadedProfiles);
    return userProfiles;
}

// LoadUserCharacters
std::vector<UserCharacters> UserManager::LoadUserCharacters(const std::string& filename)
{
    std::ifstream file(filename);
    std::vector<UserCharacters> loadedCharacters;

    if (!file.is_open()) {
        std::cerr << "파일 열기 실패: " << filename << std::endl;
        return loadedCharacters;
    }

    std::string line;
    if (!std::getline(file, line)) {
        std::cerr << "헤더 읽기 실패: " << filename << std::endl;
        return loadedCharacters;
    }

    while (std::getline(file, line)) {
        if (line.empty()) continue;

        std::stringstream ss(line);
        UserCharacters characters;

        if (std::getline(ss, characters.id, ',') &&
            std::getline(ss, line, ',') && (characters.char0 = std::stoi(line), true) &&
            std::getline(ss, line, ',') && (characters.char1 = std::stoi(line), true) &&
            std::getline(ss, line, ',') && (characters.char2 = std::stoi(line), true) &&
            std::getline(ss, line, ',') && (characters.char3 = std::stoi(line), true) &&
            std::getline(ss, line, ',') && (characters.char4 = std::stoi(line), true) &&
            std::getline(ss, line, ',') && (characters.char5 = std::stoi(line), true) &&
            std::getline(ss, line) && (characters.char6 = std::stoi(line), true))
        {
            loadedCharacters.push_back(characters);
        }
        else {
            std::cerr << "UserCharacters.csv 파싱 오류 라인: " << line << std::endl;
        }
    }

    userCharacters = std::move(loadedCharacters);
    return userCharacters;
}

// LoadUserCharacterEmotes
std::vector<UserCharacterEmotes> UserManager::LoadUserCharacterEmotes(const std::string& filename)
{
    std::ifstream file(filename);
    std::vector<UserCharacterEmotes> loadedEmotes;

    if (!file.is_open()) {
        std::cerr << "파일 열기 실패: " << filename << std::endl;
        return loadedEmotes;
    }

    std::string line;
    if (!std::getline(file, line)) {
        std::cerr << "헤더 읽기 실패: " << filename << std::endl;
        return loadedEmotes;
    }

    while (std::getline(file, line)) {
        if (line.empty()) continue;

        std::stringstream ss(line);
        UserCharacterEmotes emotes;

        if (std::getline(ss, emotes.id, ',') &&
            std::getline(ss, line, ',') && (emotes.emo0 = std::stoi(line), true) &&
            std::getline(ss, line, ',') && (emotes.emo1 = std::stoi(line), true) &&
            std::getline(ss, line, ',') && (emotes.emo2 = std::stoi(line), true) &&
            std::getline(ss, line) && (emotes.emo3 = std::stoi(line), true))
        {
            loadedEmotes.push_back(emotes);
        }
        else {
            std::cerr << "UserCharacterEmotes.csv 파싱 오류 라인: " << line << std::endl;
        }
    }

    userEmotes = std::move(loadedEmotes);
    return userEmotes;
}

std::vector<UserBallon> UserManager::LoadUserBallons(const std::string& filename)
{
    std::ifstream file(filename);
    std::vector<UserBallon> loadedBallons;

    if (!file.is_open()) {
        std::cerr << "파일 열기 실패: " << filename << std::endl;
        return loadedBallons;
    }

    std::string line;
    if (!std::getline(file, line)) {
        std::cerr << "헤더 읽기 실패: " << filename << std::endl;
        return loadedBallons;
    }

    while (std::getline(file, line)) {
        if (line.empty()) continue;

        std::stringstream ss(line);
        UserBallon ballon;

        if (std::getline(ss, ballon.id, ',') &&
            std::getline(ss, line, ',') && (ballon.balloon0 = std::stoi(line), true) &&
            std::getline(ss, line, ',') && (ballon.balloon1 = std::stoi(line), true) &&
            std::getline(ss, line, ',') && (ballon.balloon2 = std::stoi(line), true) &&
            std::getline(ss, line) && (ballon.balloon3 = std::stoi(line), true))
        {
            loadedBallons.push_back(ballon);
        }
        else {
            std::cerr << "UserBallon.csv 파싱 오류 라인: " << line << std::endl;
        }
    }

    return loadedBallons;
}


// LoadUserWinLossStats
std::vector<UserWinLossStats> UserManager::LoadUserWinLossStats(const std::string& filename)
{
    std::ifstream file(filename);
    std::vector<UserWinLossStats> loadedStats;

    if (!file.is_open()) {
        std::cerr << "파일 열기 실패: " << filename << std::endl;
        return loadedStats;
    }

    std::string line;
    if (!std::getline(file, line)) {
        std::cerr << "헤더 읽기 실패: " << filename << std::endl;
        return loadedStats;
    }

    while (std::getline(file, line)) {
        if (line.empty()) continue;

        std::stringstream ss(line);
        UserWinLossStats stats;

        if (std::getline(ss, stats.id, ',') &&
            std::getline(ss, line, ',') && (stats.winCount = std::stoi(line), true) &&
            std::getline(ss, line, ',') && (stats.LoseCount = std::stoi(line), true) &&
            std::getline(ss, line, ',') && (stats.char0_win = std::stoi(line), true) &&
            std::getline(ss, line, ',') && (stats.char0_lose = std::stoi(line), true) &&
            std::getline(ss, line, ',') && (stats.char1_win = std::stoi(line), true) &&
            std::getline(ss, line, ',') && (stats.char1_lose = std::stoi(line), true) &&
            std::getline(ss, line, ',') && (stats.char2_win = std::stoi(line), true) &&
            std::getline(ss, line, ',') && (stats.char2_lose = std::stoi(line), true) &&
            std::getline(ss, line, ',') && (stats.char3_win = std::stoi(line), true) &&
            std::getline(ss, line, ',') && (stats.char3_lose = std::stoi(line), true) &&
            std::getline(ss, line, ',') && (stats.char4_win = std::stoi(line), true) &&
            std::getline(ss, line, ',') && (stats.char4_lose = std::stoi(line), true) &&
            std::getline(ss, line, ',') && (stats.char5_win = std::stoi(line), true) &&
            std::getline(ss, line, ',') && (stats.char5_lose = std::stoi(line), true) &&
            std::getline(ss, line, ',') && (stats.char6_win = std::stoi(line), true) &&
            std::getline(ss, line) && (stats.char6_lose = std::stoi(line), true))
        {
            loadedStats.push_back(stats);
        }
        else {
            std::cerr << "UserWinLossStats.csv 파싱 오류 라인: " << line << std::endl;
        }
    }

    userStats = std::move(loadedStats);
    return userStats;
}

void UserManager::SaveUsers(const vector<UserAccount>& users, const string& filename) {
    ofstream file(filename);
    if (!file.is_open()) {
        cout << "[오류] 사용자 정보 저장 실패" << endl;
        return;
    }
    file << "id,password,nickname,level,exp\n";
    for (const auto& user : users) {
        file << user.id << "," << user.password << "," << user.nickname << "\n";
    }
}

string UserManager::CheckLogin(const string& id, const string& pw) {
    lock_guard<mutex> lock(usersMutex); // 내부에서 잠금

    for (const auto& user : users) {
        if (user.id == id) {
            if (user.password == pw) {
                stringstream ss;
                ss << "LOGIN_SUCCESS|" << user.id << "," << user.password << ","
                    << user.nickname;
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

    UserAccount newUser = { id, pw, nickname};

    // 유저 로그인 파일에 저장
    ofstream outFile("UsersAccount.csv", ios::app);
    if (!outFile.is_open())
        return "FILE_WRITE_ERROR|\n";

    outFile << newUser.id << "," << newUser.password << "," << newUser.nickname << "\n";
    outFile.close();

    // 유저 프로필 파일에 저장
    UserProfile newProfile;
    newProfile.id = id;

    ofstream profileFile("UserProfile.csv", ios::app);
    if (!profileFile.is_open())
        return "PROFILE_FILE_WRITE_ERROR|\n";

    profileFile << newProfile.id << "," << newProfile.level << "," << newProfile.exp << "," << newProfile.icon << ","
        << newProfile.money0 << "," << newProfile.money1 << ","
        << newProfile.emo0 << "," << newProfile.emo1 << "," << newProfile.emo2 << "," << newProfile.emo3 << "\n";
    profileFile.close();

    //UserCharacters 초기화 및 저장
    UserCharacters newCharacters;
    newCharacters.id = id;
    ofstream charFile("UserCharacters.csv", ios::app);
    if (!charFile.is_open())
        return "CHARACTER_FILE_WRITE_ERROR|\n";
    charFile << newCharacters.id << ","
        << newCharacters.char0 << "," << newCharacters.char1 << "," << newCharacters.char2 << ","
        << newCharacters.char3 << "," << newCharacters.char4 << "," << newCharacters.char5 << ","
        << newCharacters.char6 << "\n";
    charFile.close();

    // UserCharacterEmotes 초기화 및 저장
    UserCharacterEmotes newEmotes;
    newEmotes.id = id;
    ofstream emoteFile("UserCharacterEmotes.csv", ios::app);
    if (!emoteFile.is_open())
        return "EMOTE_FILE_WRITE_ERROR|\n";
    emoteFile << newEmotes.id << ","
        << newEmotes.emo0 << "," << newEmotes.emo1 << "," << newEmotes.emo2 << "," << newEmotes.emo3 << "\n";
    emoteFile.close();

    // UserWinLossStats 초기화 및 저장
    UserWinLossStats newStats;
    newStats.id = id;
    ofstream statsFile("UserWinLossStats.csv", ios::app);
    if (!statsFile.is_open())
        return "STATS_FILE_WRITE_ERROR|\n";
    statsFile << newStats.id << ","
        << newStats.winCount << "," << newStats.LoseCount << ","
        << newStats.char0_win << "," << newStats.char0_lose << ","
        << newStats.char1_win << "," << newStats.char1_lose << ","
        << newStats.char2_win << "," << newStats.char2_lose << ","
        << newStats.char3_win << "," << newStats.char3_lose << ","
        << newStats.char4_win << "," << newStats.char4_lose << ","
        << newStats.char5_win << "," << newStats.char5_lose << ","
        << newStats.char6_win << "," << newStats.char6_lose << "\n";
    statsFile.close();

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

            auto itUser = std::find_if(users.begin(), users.end(), [&](const UserAccount& u) {
                return u.id == c->id;
                });

            if (itUser != users.end()) {
                const std::string& nickname = itUser->nickname;

                // UserProfile에서 레벨 찾기
                auto itProfile = std::find_if(userProfiles.begin(), userProfiles.end(), [&](const UserProfile& p) {
                    return p.id == c->id;
                    });

                int level = 1;  // 기본 레벨
                if (itProfile != userProfiles.end()) {
                    level = itProfile->level;
                }

                message += "|" + nickname + "," + std::to_string(level);
            }
            else {
                // UserAccount가 없으면 id로 기본 레벨 1 전송
                message += "|" + c->id + ",1";
            }
        }

        message += "\n";

        for (const auto& c : clients) {
            if (c->id.empty()) continue;
            int sendLen = send(c->socket, message.c_str(), (int)message.size(), 0);
            if (sendLen == SOCKET_ERROR) {
                std::cout << "[BroadcastLobbyUserList] send 오류 - user: "
                    << c->id << ", 오류 코드: " << WSAGetLastError() << std::endl;
            }
        }
    }

    cout << "[BroadcastLobbyUserList] 메시지 전송 완료: " << message;
}

void UserManager::SendUserInfoByNickname(shared_ptr<ClientInfo> client, const string& nickname)
{
    lock_guard<mutex> lock(usersMutex);

    auto itUser = find_if(users.begin(), users.end(), [&](const UserAccount& u) {
        return u.nickname == nickname;
        });

    if (itUser == users.end())
    {
        string response = "USER_NOT_FOUND|\n";
        send(client->socket, response.c_str(), (int)response.size(), 0);
        return;
    }
    const UserAccount& user = *itUser;

    auto itProfile = find_if(userProfiles.begin(), userProfiles.end(), [&](const UserProfile& p) {
        return p.id == user.id;
        });

    if (itProfile == userProfiles.end())
    {
        string response = "PROFILE_NOT_FOUND|\n";
        send(client->socket, response.c_str(), (int)response.size(), 0);
        return;
    }

    const UserProfile& profile = *itProfile;

    string response = "USER_INFO|" + user.nickname + "," +
        to_string(profile.level) + "," +
        to_string(profile.exp) + "\n";

    send(client->socket, response.c_str(), (int)response.size(), 0);
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

    // clients 리스트에서 해당 클라이언트 정보 초기화
    for (auto& c : clients) {
        if (c->socket == client->socket) {
            c->id.clear();      // 닉네임 초기화
            break;
        }
    }

    std::cout << "[UserManager] 유저 로그아웃 처리 완료: " << client->id << std::endl;
    BroadcastLobbyUserList();
}

std::vector<int> UserManager::GetEmotionsByUserId(const std::string& userId) {
    std::ifstream file("UserProfile.csv");
    if (!file.is_open()) {
        std::cerr << "[GET_EMO] UserProfile.csv 열기 실패" << std::endl;
        return {};
    }

    std::string ID = Trim(userId);
    std::string line;
    if (!std::getline(file, line)) return {}; // 헤더 스킵

    while (std::getline(file, line)) {
        std::stringstream ss(line);
        std::string id, levelStr, expStr, iconStr, money0Str, money1Str;
        std::string emo0, emo1, emo2, emo3, balloon;

        if (std::getline(ss, id, ',') &&
            std::getline(ss, levelStr, ',') &&
            std::getline(ss, expStr, ',') &&
            std::getline(ss, iconStr, ',') &&
            std::getline(ss, money0Str, ',') &&
            std::getline(ss, money1Str, ',') &&
            std::getline(ss, emo0, ',') &&
            std::getline(ss, emo1, ',') &&
            std::getline(ss, emo2, ',') &&
            std::getline(ss, emo3, ',') &&
            std::getline(ss, balloon, ',')) {

            if (Trim(id) == ID) {
                try {
                    return {
                        std::stoi(emo0),
                        std::stoi(emo1),
                        std::stoi(emo2),
                        std::stoi(emo3)
                    };
                }
                catch (const std::exception& e) {
                    std::cerr << "[GET_EMO] stoi 변환 실패: " << e.what() << std::endl;
                    return {};
                }
            }
        }
    }

    std::cerr << "[GET_EMO] 일치하는 ID 없음: " << userId << std::endl;
    return {};
}

int UserManager::GetBalloonByUserId(const std::string& userId) {
    std::ifstream file("UserProfile.csv");
    if (!file.is_open()) {
        std::cerr << "[GET_BALLOON] UserProfile.csv 열기 실패" << std::endl;
        return -1;
    }

    std::string ID = Trim(userId);
    std::string line;
    if (!std::getline(file, line)) return -1; // 헤더 스킵

    while (std::getline(file, line)) {
        std::stringstream ss(line);
        std::string id, levelStr, expStr, iconStr, money0Str, money1Str;
        std::string emo0, emo1, emo2, emo3, balloonStr;

        if (std::getline(ss, id, ',') &&
            std::getline(ss, levelStr, ',') &&
            std::getline(ss, expStr, ',') &&
            std::getline(ss, iconStr, ',') &&
            std::getline(ss, money0Str, ',') &&
            std::getline(ss, money1Str, ',') &&
            std::getline(ss, emo0, ',') &&
            std::getline(ss, emo1, ',') &&
            std::getline(ss, emo2, ',') &&
            std::getline(ss, emo3, ',') &&
            std::getline(ss, balloonStr)) {

            if (Trim(id) == ID) {
                try {
                    return std::stoi(balloonStr); // 딱 하나만 반환
                }
                catch (const std::exception& e) {
                    std::cerr << "[GET_BALLOON] stoi 실패: " << e.what() << std::endl;
                    return -1;
                }
            }
        }
    }

    std::cerr << "[GET_BALLOON] 일치하는 ID 없음: " << userId << std::endl;
    return -1;
}

std::vector<int> UserManager::GetCharactersByUserId(const std::string& userId)
{
    std::ifstream file("UserCharacters.csv");
    std::string line;

    // 첫 줄은 헤더니까 skip
    std::getline(file, line);

    while (std::getline(file, line)) {
        std::stringstream ss(line);
        std::string id;
        std::getline(ss, id, ',');

        if (Trim(id) == userId) {
            std::vector<int> characters;
            std::string token;
            while (std::getline(ss, token, ',')) {
                characters.push_back(std::stoi(token));
            }
            return characters;
        }
    }

    return {}; // 찾지 못한 경우
}
