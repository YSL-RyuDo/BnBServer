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

std::vector<UserCharacterEmotes> UserManager::LoadUserEmotes(const std::string& filename)
{
    std::ifstream file(filename);
    std::vector<UserCharacterEmotes> loadedEmotes;

    if (!file.is_open()) {
        std::cerr << "파일 열기 실패: " << filename << std::endl;
        return loadedEmotes;
    }

    std::string line;

    // 헤더 읽기
    if (!std::getline(file, line)) {
        std::cerr << "헤더 읽기 실패: " << filename << std::endl;
        return loadedEmotes;
    }

    while (std::getline(file, line)) {
        if (line.empty()) continue;

        std::stringstream ss(line);
        UserCharacterEmotes emotes;

        // ID 읽기
        if (!std::getline(ss, emotes.id, ',')) {
            std::cerr << "UserCharacterEmotes.csv 파싱 오류 (ID) 라인: " << line << std::endl;
            continue;
        }

        // emo0~emo35 읽기
        for (int i = 0; i < 36; ++i) {
            std::string value;
            if (!std::getline(ss, value, ',')) {
                std::cerr << "UserCharacterEmotes.csv 파싱 오류 (emo" << i << ") 라인: " << line << std::endl;
                break;
            }

            int intValue = std::stoi(value);
            switch (i) {
            case 0: emotes.emo0 = intValue; break;
            case 1: emotes.emo1 = intValue; break;
            case 2: emotes.emo2 = intValue; break;
            case 3: emotes.emo3 = intValue; break;
            case 4: emotes.emo4 = intValue; break;
            case 5: emotes.emo5 = intValue; break;
            case 6: emotes.emo6 = intValue; break;
            case 7: emotes.emo7 = intValue; break;
            case 8: emotes.emo8 = intValue; break;
            case 9: emotes.emo9 = intValue; break;
            case 10: emotes.emo10 = intValue; break;
            case 11: emotes.emo11 = intValue; break;
            case 12: emotes.emo12 = intValue; break;
            case 13: emotes.emo13 = intValue; break;
            case 14: emotes.emo14 = intValue; break;
            case 15: emotes.emo15 = intValue; break;
            case 16: emotes.emo16 = intValue; break;
            case 17: emotes.emo17 = intValue; break;
            case 18: emotes.emo18 = intValue; break;
            case 19: emotes.emo19 = intValue; break;
            case 20: emotes.emo20 = intValue; break;
            case 21: emotes.emo21 = intValue; break;
            case 22: emotes.emo22 = intValue; break;
            case 23: emotes.emo23 = intValue; break;
            case 24: emotes.emo24 = intValue; break;
            case 25: emotes.emo25 = intValue; break;
            case 26: emotes.emo26 = intValue; break;
            case 27: emotes.emo27 = intValue; break;
            case 28: emotes.emo28 = intValue; break;
            case 29: emotes.emo29 = intValue; break;
            case 30: emotes.emo30 = intValue; break;
            case 31: emotes.emo31 = intValue; break;
            case 32: emotes.emo32 = intValue; break;
            case 33: emotes.emo33 = intValue; break;
            case 34: emotes.emo34 = intValue; break;
            case 35: emotes.emo35 = intValue; break;
            }
        }

        loadedEmotes.push_back(emotes);
    }

    // 멤버 벡터에 저장
    userEmotes = std::move(loadedEmotes);
    return userEmotes;
}

// UserManager.cpp
std::vector<UserBallons> UserManager::LoadUserBallons(const std::string& filename)
{
    std::ifstream file(filename);
    std::vector<UserBallons> loadedBallons;

    if (!file.is_open()) return loadedBallons;

    std::string line;
    std::getline(file, line); // header skip

    while (std::getline(file, line))
    {
        if (line.empty()) continue;

        std::stringstream ss(line);
        UserBallons b;
        std::string tmp;

        std::getline(ss, b.id, ',');
        std::getline(ss, tmp, ','); b.balloon0 = std::stoi(tmp);
        std::getline(ss, tmp, ','); b.balloon1 = std::stoi(tmp);
        std::getline(ss, tmp, ','); b.balloon2 = std::stoi(tmp);
        std::getline(ss, tmp, ','); b.balloon3 = std::stoi(tmp);
        std::getline(ss, tmp, ','); b.balloon4 = std::stoi(tmp);
        std::getline(ss, tmp, ','); b.balloon5 = std::stoi(tmp);
        std::getline(ss, tmp, ','); b.balloon6 = std::stoi(tmp);
        std::getline(ss, tmp, ','); b.balloon7 = std::stoi(tmp);
        std::getline(ss, tmp, ','); b.balloon8 = std::stoi(tmp);
        std::getline(ss, tmp);      b.balloon9 = std::stoi(tmp);

        loadedBallons.push_back(b);
    }

    userBallons = std::move(loadedBallons);
    return userBallons;
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
            std::getline(ss, line, ',') && (stats.loseCount = std::stoi(line), true) &&
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

vector<UserIcons> UserManager::LoadUserIcons(const string& filename)
{
    std::ifstream file(filename);
    std::vector<UserIcons> loadedIcons;

    if (!file.is_open()) {
        std::cerr << "파일 열기 실패: " << filename << std::endl;
        return loadedIcons;
    }

    std::string line;
    // 첫 줄은 헤더 (무시)
    if (!std::getline(file, line)) {
        std::cerr << "헤더 읽기 실패: " << filename << std::endl;
        return loadedIcons;
    }

    while (std::getline(file, line)) {
        if (line.empty()) continue;

        std::stringstream ss(line);
        UserIcons icon;

        if (std::getline(ss, icon.id, ',') &&
            std::getline(ss, line, ',') && (icon.icon0 = std::stoi(line), true) &&
            std::getline(ss, line, ',') && (icon.icon1 = std::stoi(line), true) &&
            std::getline(ss, line, ',') && (icon.icon2 = std::stoi(line), true) &&
            std::getline(ss, line, ',') && (icon.icon3 = std::stoi(line), true) &&
            std::getline(ss, line, ',') && (icon.icon4 = std::stoi(line), true) &&
            std::getline(ss, line, ',') && (icon.icon5 = std::stoi(line), true) &&
            std::getline(ss, line, ',') && (icon.icon6 = std::stoi(line), true) &&
            std::getline(ss, line, ',') && (icon.icon7 = std::stoi(line), true) &&
            std::getline(ss, line, ',') && (icon.icon8 = std::stoi(line), true) &&
            std::getline(ss, line) && (icon.icon9 = std::stoi(line), true))
        {
            loadedIcons.push_back(icon);
        }
        else {
            std::cerr << "UserIcon.csv 파싱 오류 라인: " << line << std::endl;
        }
    }

    userIcons = std::move(loadedIcons);
    return loadedIcons;
}

bool UserManager::RegisterUserAccount(const UserAccount& user) {
    std::ofstream outFile("UsersAccount.csv", std::ios::app);
    if (!outFile.is_open()) return false;

    outFile << user.id << "," << user.password << "," << user.nickname << "\n";
    return true;
}
bool UserManager::RegisterUserProfile(const UserProfile& profile) {
    std::ofstream profileFile("UserProfile.csv", std::ios::app);
    if (!profileFile.is_open()) return false;

    profileFile << profile.id << "," << profile.level << "," << profile.exp << "," << profile.icon << ","
        << profile.money0 << "," << profile.money1 << ","
        << profile.emo0 << "," << profile.emo1 << "," << profile.emo2 << "," << profile.emo3 << ","
        << profile.balloon << "\n";
    return true;
}
bool UserManager::RegisterUserCharacters(const UserCharacters& characters) {
    std::ofstream charFile("UserCharacters.csv", std::ios::app);
    if (!charFile.is_open()) return false;

    charFile << characters.id << ","
        << characters.char0 << "," << characters.char1 << "," << characters.char2 << ","
        << characters.char3 << "," << characters.char4 << "," << characters.char5 << ","
        << characters.char6 << "\n";
    return true;
}
bool UserManager::RegisterUserEmotes(const UserCharacterEmotes& emotes) {
    std::ofstream emoteFile("UserEmotes.csv", std::ios::app);
    if (!emoteFile.is_open()) return false;

    emoteFile << emotes.id << ","
        << emotes.emo0 << "," << emotes.emo1 << "," << emotes.emo2 << "," << emotes.emo3 << ","
        << emotes.emo4 << "," << emotes.emo5 << "," << emotes.emo6 << "," << emotes.emo7 << ","
        << emotes.emo8 << "," << emotes.emo9 << "," << emotes.emo10 << "," << emotes.emo11 << ","
        << emotes.emo12 << "," << emotes.emo13 << "," << emotes.emo14 << "," << emotes.emo15 << ","
        << emotes.emo16 << "," << emotes.emo17 << "," << emotes.emo18 << "," << emotes.emo19 << ","
        << emotes.emo20 << "," << emotes.emo21 << "," << emotes.emo22 << "," << emotes.emo23 << ","
        << emotes.emo24 << "," << emotes.emo25 << "," << emotes.emo26 << "," << emotes.emo27 << ","
        << emotes.emo28 << "," << emotes.emo29 << "," << emotes.emo30 << "," << emotes.emo31 << ","
        << emotes.emo32 << "," << emotes.emo33 << "," << emotes.emo34 << "," << emotes.emo35
        << "\n";

    return true;
}
bool UserManager::RegisterUserWinLossStats(const UserWinLossStats& stats) {
    std::ofstream statsFile("UserWinLossStats.csv", std::ios::app);
    if (!statsFile.is_open()) return false;

    statsFile << stats.id << ","
        << stats.winCount << "," << stats.loseCount << ","
        << stats.char0_win << "," << stats.char0_lose << ","
        << stats.char1_win << "," << stats.char1_lose << ","
        << stats.char2_win << "," << stats.char2_lose << ","
        << stats.char3_win << "," << stats.char3_lose << ","
        << stats.char4_win << "," << stats.char4_lose << ","
        << stats.char5_win << "," << stats.char5_lose << ","
        << stats.char6_win << "," << stats.char6_lose
        << "\n";

    return true;
}
bool UserManager::RegisterUserBallons(const UserBallons& ballon) {
    std::ofstream ballonFile("UserBalloon.csv", std::ios::app);
    if (!ballonFile.is_open()) return false;

    ballonFile << ballon.id << ","
        << ballon.balloon0 << "," << ballon.balloon1 << "," << ballon.balloon2 << "," << ballon.balloon3 << ","
        << ballon.balloon4 << "," << ballon.balloon5 << "," << ballon.balloon6 << "," << ballon.balloon7 << ","
        << ballon.balloon8 << "," << ballon.balloon9
        << "\n";

    return true;
}
bool UserManager::RegisterUserIcons(const UserIcons& icon) {
    std::ofstream iconFile("UserIcon.csv", std::ios::app);
    if (!iconFile.is_open()) return false;

    iconFile << icon.id << ","
        << icon.icon0 << "," << icon.icon1 << "," << icon.icon2 << "," << icon.icon3 << ","
        << icon.icon4 << "," << icon.icon5 << "," << icon.icon6 << "," << icon.icon7 << ","
        << icon.icon8 << "," << icon.icon9
        << "\n";

    return true;
}

// Users.csv
bool UserManager::SaveUserAccount(const std::string& userId)
{
    std::ifstream inFile("UsersAccount.csv");
    if (!inFile.is_open()) return false;
    std::ofstream tempFile("temp.csv");
    if (!tempFile.is_open()) return false;

    std::string line;
    bool header = true;
    while (std::getline(inFile, line))
    {
        if (header)
        {
            tempFile << line << "\n";
            header = false;
            continue;
        }

        std::string id = line.substr(0, line.find(','));
        if (id == userId)
        {
            auto it = std::find_if(users.begin(), users.end(), [&](const UserAccount& u) { return u.id == userId; });
            if (it != users.end())
                tempFile << it->id << "," << it->password << "," << it->nickname << "\n";
        }
        else tempFile << line << "\n";
    }

    inFile.close();
    tempFile.close();
    std::remove("UsersAccount.csv");
    std::rename("temp.csv", "UsersAccount.csv");
    return true;
}

// UserProfiles.csv
bool UserManager::SaveUserProfile(const std::string& userId)
{
    std::ifstream inFile("UserProfile.csv");
    if (!inFile.is_open()) return false;
    std::ofstream tempFile("temp.csv");
    if (!tempFile.is_open()) return false;

    std::string line;
    bool header = true;
    while (std::getline(inFile, line))
    {
        if (header)
        {
            tempFile << line << "\n";
            header = false;
            continue;
        }

        std::string id = line.substr(0, line.find(','));
        if (id == userId)
        {
            auto it = std::find_if(userProfiles.begin(), userProfiles.end(), [&](const UserProfile& p) { return p.id == userId; });
            if (it != userProfiles.end())
                tempFile << it->id << "," << it->level << "," << it->exp << "," << it->icon << ","
                << it->money0 << "," << it->money1 << ","
                << it->emo0 << "," << it->emo1 << "," << it->emo2 << "," << it->emo3 << ","
                << it->balloon << "\n";
        }
        else tempFile << line << "\n";
    }

    inFile.close();
    tempFile.close();
    std::remove("UserProfile.csv");
    std::rename("temp.csv", "UserProfile.csv");
    return true;
}

// UserCharacters.csv
bool UserManager::SaveUserCharacters(const std::string& userId)
{
    std::ifstream inFile("UserCharacters.csv");
    if (!inFile.is_open()) return false;
    std::ofstream tempFile("temp.csv");
    if (!tempFile.is_open()) return false;

    std::string line;
    bool header = true;
    while (std::getline(inFile, line))
    {
        if (header)
        {
            tempFile << line << "\n";
            header = false;
            continue;
        }

        std::string id = line.substr(0, line.find(','));
        if (id == userId)
        {
            auto it = std::find_if(userCharacters.begin(), userCharacters.end(), [&](const UserCharacters& c) { return c.id == userId; });
            if (it != userCharacters.end())
                tempFile << it->id << "," << it->char0 << "," << it->char1 << "," << it->char2 << ","
                << it->char3 << "," << it->char4 << "," << it->char5 << "," << it->char6 << "\n";
        }
        else tempFile << line << "\n";
    }

    inFile.close();
    tempFile.close();
    std::remove("UserCharacters.csv");
    std::rename("temp.csv", "UserCharacters.csv");
    return true;
}

// UserEmotes.csv
bool UserManager::SaveUserEmotes(const std::string& userId)
{
    std::ifstream inFile("UserEmotes.csv");
    if (!inFile.is_open()) return false;
    std::ofstream tempFile("temp.csv");
    if (!tempFile.is_open()) return false;

    std::string line;
    bool header = true;
    while (std::getline(inFile, line))
    {
        if (header)
        {
            tempFile << line << "\n";
            header = false;
            continue;
        }

        std::string id = line.substr(0, line.find(','));
        if (id == userId)
        {
            auto it = std::find_if(userEmotes.begin(), userEmotes.end(), [&](const UserCharacterEmotes& e) { return e.id == userId; });
            if (it != userEmotes.end())
            {
                tempFile << it->id
                    << "," << it->emo0 << "," << it->emo1 << "," << it->emo2 << "," << it->emo3
                    << "," << it->emo4 << "," << it->emo5 << "," << it->emo6 << "," << it->emo7
                    << "," << it->emo8 << "," << it->emo9 << "," << it->emo10 << "," << it->emo11
                    << "," << it->emo12 << "," << it->emo13 << "," << it->emo14 << "," << it->emo15
                    << "," << it->emo16 << "," << it->emo17 << "," << it->emo18 << "," << it->emo19
                    << "," << it->emo20 << "," << it->emo21 << "," << it->emo22 << "," << it->emo23
                    << "," << it->emo24 << "," << it->emo25 << "," << it->emo26 << "," << it->emo27
                    << "," << it->emo28 << "," << it->emo29 << "," << it->emo30 << "," << it->emo31
                    << "," << it->emo32 << "," << it->emo33 << "," << it->emo34 << "," << it->emo35
                    << "\n";

            }
        }
        else tempFile << line << "\n";
    }

    inFile.close();
    tempFile.close();
    std::remove("UserEmotes.csv");
    std::rename("temp.csv", "UserEmotes.csv");
    return true;
}

// UserWinLossStats.csv
bool UserManager::SaveUserWinLossStats(const std::string& userId)
{
    std::ifstream inFile("UserWinLossStats.csv");
    if (!inFile.is_open()) return false;
    std::ofstream tempFile("temp.csv");
    if (!tempFile.is_open()) return false;

    std::string line;
    bool header = true;
    while (std::getline(inFile, line))
    {
        if (header)
        {
            tempFile << line << "\n";
            header = false;
            continue;
        }

        std::string id = line.substr(0, line.find(','));
        if (id == userId)
        {
            auto it = std::find_if(userStats.begin(), userStats.end(), [&](const UserWinLossStats& s) { return s.id == userId; });
            if (it != userStats.end())
                tempFile << it->id << "," << it->winCount << "," << it->loseCount << ","
                << it->char0_win << "," << it->char0_lose << "," << it->char1_win << "," << it->char1_lose << ","
                << it->char2_win << "," << it->char2_lose << "," << it->char3_win << "," << it->char3_lose << ","
                << it->char4_win << "," << it->char4_lose << "," << it->char5_win << "," << it->char5_lose << ","
                << it->char6_win << "," << it->char6_lose << "\n";
        }
        else tempFile << line << "\n";
    }

    inFile.close();
    tempFile.close();
    std::remove("UserWinLossStats.csv");
    std::rename("temp.csv", "UserWinLossStats.csv");
    return true;
}

// UserBallons.csv
bool UserManager::SaveUserBallons(const std::string& userId)
{
    std::ifstream inFile("UserBalloon.csv");
    if (!inFile.is_open()) return false;
    std::ofstream tempFile("temp.csv");
    if (!tempFile.is_open()) return false;

    std::string line;
    bool header = true;
    while (std::getline(inFile, line))
    {
        if (header)
        {
            tempFile << line << "\n";
            header = false;
            continue;
        }

        std::string id = line.substr(0, line.find(','));
        if (id == userId)
        {
            auto it = std::find_if(userBallons.begin(), userBallons.end(), [&](const UserBallons& b) { return b.id == userId; });
            if (it != userBallons.end())
                tempFile << it->id << "," << it->balloon0 << "," << it->balloon1 << "," << it->balloon2 << "," << it->balloon3
                << "," << it->balloon4 << "," << it->balloon5 << "," << it->balloon6 << "," << it->balloon7
                << "," << it->balloon8 << "," << it->balloon9 << "\n";
        }
        else tempFile << line << "\n";
    }

    inFile.close();
    tempFile.close();
    std::remove("UserBalloon.csv");
    std::rename("temp.csv", "UserBalloon.csv");
    return true;
}

// UserIcons.csv
bool UserManager::SaveUserIcons(const std::string& userId)
{
    std::ifstream inFile("UserIcon.csv");
    if (!inFile.is_open()) return false;
    std::ofstream tempFile("temp.csv");
    if (!tempFile.is_open()) return false;

    std::string line;
    bool header = true;
    while (std::getline(inFile, line))
    {
        if (header)
        {
            tempFile << line << "\n";
            header = false;
            continue;
        }

        std::string id = line.substr(0, line.find(','));
        if (id == userId)
        {
            auto it = std::find_if(userIcons.begin(), userIcons.end(), [&](const UserIcons& i) { return i.id == userId; });
            if (it != userIcons.end())
                tempFile << it->id << "," << it->icon0 << "," << it->icon1 << "," << it->icon2 << "," << it->icon3
                << "," << it->icon4 << "," << it->icon5 << "," << it->icon6 << "," << it->icon7
                << "," << it->icon8 << "," << it->icon9 << "\n";
        }
        else tempFile << line << "\n";
    }

    inFile.close();
    tempFile.close();
    std::remove("UserIcon.csv");
    std::rename("temp.csv", "UserIcon.csv");
    return true;
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

    // 중복 체크
    for (const auto& user : users) {
        if (user.id == id) return "DUPLICATE_ID|\n";
        if (user.nickname == nickname) return "DUPLICATE_NICK|\n";
    }

    if (pw.empty()) return "EMPTY_PASSWORD|\n";

    UserAccount newUser{ id, pw, nickname };
    if (!RegisterUserAccount(newUser)) return "FILE_WRITE_ERROR|\n";

    UserProfile newProfile;
    newProfile.id = id;
    if (!RegisterUserProfile(newProfile)) return "PROFILE_FILE_WRITE_ERROR|\n";

    UserCharacters newCharacters;
    newCharacters.id = id;
    if (!RegisterUserCharacters(newCharacters)) return "CHARACTER_FILE_WRITE_ERROR|\n";

    UserCharacterEmotes newEmotes;
    newEmotes.id = id;
    if (!RegisterUserEmotes(newEmotes)) return "EMOTE_FILE_WRITE_ERROR|\n";

    UserWinLossStats newStats;
    newStats.id = id;
    if (!RegisterUserWinLossStats(newStats)) return "STATS_FILE_WRITE_ERROR|\n";

    UserBallons newBallon;
    newBallon.id = id;
    if (!RegisterUserBallons(newBallon)) return "BALLOON_FILE_WRITE_ERROR|\n";

    UserIcons newIcon;
    newIcon.id = id;
    if (!RegisterUserIcons(newIcon)) return "ICON_FILE_WRITE_ERROR|\n";

    // 메모리 상 컨테이너에 추가
    users.push_back(newUser);
    userProfiles.push_back(newProfile);
    userCharacters.push_back(newCharacters);
    userEmotes.push_back(newEmotes);
    userStats.push_back(newStats);
    userBallons.push_back(newBallon);
    userIcons.push_back(newIcon);

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

void UserManager::LogoutUser(std::shared_ptr<ClientInfo> client)
{
    std::string userId = client->id;

    {
        std::lock_guard<std::mutex> lock(server_.GetClientsMutex());
        client->id.clear();
    }

    // 핵심 추가
    if (!userId.empty())
    {
        clientHandler_->RemoveLoginSession(userId);
    }

    std::cout << "[UserManager] 유저 로그아웃 처리 완료: " << userId << std::endl;

    BroadcastLobbyUserList();
}


//void UserManager::LogoutUser(std::shared_ptr<ClientInfo> client) {
//    std::lock_guard<std::mutex> lock(server_.GetClientsMutex());
//
//    auto& clients = server_.GetClients();
//
//    // clients 리스트에서 해당 클라이언트 정보 초기화
//    for (auto& c : clients) {
//        if (c->socket == client->socket) {
//            c->id.clear();      // 닉네임 초기화
//            break;
//        }
//    }
//
//    std::cout << "[UserManager] 유저 로그아웃 처리 완료: " << client->id << std::endl;
//    BroadcastLobbyUserList();
//}

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

UserProfile& UserManager::GetUserProfileById(const std::string& id)
{
    std::lock_guard<std::mutex> lock(usersMutex);

    for (auto& profile : userProfiles)
    {
        if (profile.id == id)
        {
            return profile;
        }
    }

    throw std::runtime_error("UserProfile not found for id: " + id);
}

void UserManager::UpdateWinLoss(const std::string& userId, bool isWin, int charIndex)
{
    auto it = std::find_if(userStats.begin(), userStats.end(),
        [&](const UserWinLossStats& s) { return s.id == userId; });
    if (it == userStats.end())
        return;

    if (isWin)
    {
        it->winCount++;
        switch (charIndex)
        {
        case 0: it->char0_win++; break;
        case 1: it->char1_win++; break;
        case 2: it->char2_win++; break;
        case 3: it->char3_win++; break;
        case 4: it->char4_win++; break;
        case 5: it->char5_win++; break;
        case 6: it->char6_win++; break;
        }
    }
    else
    {
        it->loseCount++;
        switch (charIndex)
        {
        case 0: it->char0_lose++; break;
        case 1: it->char1_lose++; break;
        case 2: it->char2_lose++; break;
        case 3: it->char3_lose++; break;
        case 4: it->char4_lose++; break;
        case 5: it->char5_lose++; break;
        case 6: it->char6_lose++; break;
        }
    }
}

UserWinLossStats& UserManager::GetUserWinLossStatsById(const std::string& id) {
    for (auto& stats : userStats) {
        if (stats.id == id)
            return stats;
    }

    // ID가 존재하지 않으면 새 객체를 추가 후 반환
    UserWinLossStats newStats;
    newStats.id = id;
    userStats.push_back(newStats);
    return userStats.back();
}

UserCharacterEmotes& UserManager::GetUserEmotesById(const std::string& userId) {
    // 기존 벡터에서 검색
    for (auto& emotes : userEmotes) {
        if (emotes.id == userId)
            return emotes;
    }

    // ID가 존재하지 않으면 새 객체를 추가 후 반환
    UserCharacterEmotes newEmotes;
    newEmotes.id = userId;

    // 모든 이모티콘 0으로 초기화 (미보유)
    for (int i = 0; i < 36; ++i) {
        switch (i) {
        case 0: newEmotes.emo0 = 0; break;
        case 1: newEmotes.emo1 = 0; break;
        case 2: newEmotes.emo2 = 0; break;
        case 3: newEmotes.emo3 = 0; break;
        case 4: newEmotes.emo4 = 0; break;
        case 5: newEmotes.emo5 = 0; break;
        case 6: newEmotes.emo6 = 0; break;
        case 7: newEmotes.emo7 = 0; break;
        case 8: newEmotes.emo8 = 0; break;
        case 9: newEmotes.emo9 = 0; break;
        case 10: newEmotes.emo10 = 0; break;
        case 11: newEmotes.emo11 = 0; break;
        case 12: newEmotes.emo12 = 0; break;
        case 13: newEmotes.emo13 = 0; break;
        case 14: newEmotes.emo14 = 0; break;
        case 15: newEmotes.emo15 = 0; break;
        case 16: newEmotes.emo16 = 0; break;
        case 17: newEmotes.emo17 = 0; break;
        case 18: newEmotes.emo18 = 0; break;
        case 19: newEmotes.emo19 = 0; break;
        case 20: newEmotes.emo20 = 0; break;
        case 21: newEmotes.emo21 = 0; break;
        case 22: newEmotes.emo22 = 0; break;
        case 23: newEmotes.emo23 = 0; break;
        case 24: newEmotes.emo24 = 0; break;
        case 25: newEmotes.emo25 = 0; break;
        case 26: newEmotes.emo26 = 0; break;
        case 27: newEmotes.emo27 = 0; break;
        case 28: newEmotes.emo28 = 0; break;
        case 29: newEmotes.emo29 = 0; break;
        case 30: newEmotes.emo30 = 0; break;
        case 31: newEmotes.emo31 = 0; break;
        case 32: newEmotes.emo32 = 0; break;
        case 33: newEmotes.emo33 = 0; break;
        case 34: newEmotes.emo34 = 0; break;
        case 35: newEmotes.emo35 = 0; break;
        }
    }

    userEmotes.push_back(newEmotes);
    return userEmotes.back();
}

UserBallons& UserManager::GetUserBallonsById(const std::string& id)
{
    for (auto& b : userBallons)
        if (b.id == id) return b;

    UserBallons newBallon;
    newBallon.id = id;
    userBallons.push_back(newBallon);
    return userBallons.back();
}

UserIcons& UserManager::GetUserIconsById(const std::string& userId)
{
    for (auto& icon : userIcons)
    {
        if (icon.id == userId)
            return icon;
    }

    // 없으면 새로 생성
    UserIcons newIcon;
    newIcon.id = userId;
    userIcons.push_back(newIcon);
    return userIcons.back();
}

UserCharacters& UserManager::GetUserCharactersById(const std::string& userId)
{
    for (auto& c : userCharacters)
    {
        if (c.id == userId)
            return c;
    }

    // 없으면 새로 생성
    UserCharacters newChar;
    newChar.id = userId;
    userCharacters.push_back(newChar);
    return userCharacters.back();
}


int UserManager::GetAttackByIndex(int index)
{
    std::ifstream file("CharStat.csv");
    if (!file.is_open()) {
        std::cerr << "[GetAttackByIndex] CharStat.csv 열기 실패" << std::endl;
        return 0;
    }

    std::string line;
    if (!std::getline(file, line)) {
        std::cerr << "[GetAttackByIndex] 헤더 읽기 실패" << std::endl;
        return 0;
    }

    while (std::getline(file, line)) {
        if (line.empty()) continue;

        std::stringstream ss(line);
        std::string indexStr, healthStr, attackStr;

        if (std::getline(ss, indexStr, ',') &&
            std::getline(ss, healthStr, ',') &&
            std::getline(ss, attackStr, ',')) {

            try {
                int currentIndex = std::stoi(indexStr);
                if (currentIndex == index) {
                    return std::stoi(attackStr);
                }
            }
            catch (const std::exception& e) {
                std::cerr << "[GetAttackByIndex] 변환 오류: " << e.what() << " | 라인: " << line << std::endl;
                return 0;
            }
        }
    }

    std::cerr << "[GetAttackByIndex] 인덱스 " << index << "에 해당하는 공격력 없음" << std::endl;
    return 0;
}
