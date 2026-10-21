#pragma once
#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>
#include <mutex>
#include <unordered_map>
#include "CsvStore.h"
#include "UserAccount.h"
#include "Utils.h"
#include "ClientInfo.h"

class Server;
class ClientHandler;

class UserManager
{
public:
	UserManager(Server& server);
    // Borrowed user references are valid only while this guard is alive.
    std::unique_lock<std::recursive_mutex> LockState() { return std::unique_lock<std::recursive_mutex>(usersMutex); }
    class Transaction {
        UserManager& owner_;
        std::unique_lock<std::recursive_mutex> lock_;
        vector<UserAccount> users_; vector<UserProfile> profiles_;
        vector<UserCharacters> characters_; vector<UserCharacterEmotes> emotes_;
        vector<UserWinLossStats> stats_; vector<UserBallons> balloons_; vector<UserIcons> icons_;
        bool committed_ = false;
    public:
        explicit Transaction(UserManager& owner);
        ~Transaction();
        bool Commit();
        Transaction(const Transaction&) = delete;
        Transaction& operator=(const Transaction&) = delete;
    };
	void SetClientHandler(ClientHandler* handler) { clientHandler_ = handler; }

	vector<UserAccount> LoadAccountUsers(const string&);
	vector<UserProfile> LoadUserProfiles(const string&);
	vector<UserCharacters> LoadUserCharacters(const string&);
	vector<UserCharacterEmotes> LoadUserEmotes(const string&);
	vector<UserBallons> LoadUserBallons(const std::string& filename);
	vector<UserWinLossStats> LoadUserWinLossStats(const string&);
	vector<UserIcons> LoadUserIcons(const string& filename);

	bool RegisterUserAccount(const UserAccount& user);
	bool RegisterUserProfile(const UserProfile& profile);
	bool RegisterUserCharacters(const UserCharacters& characters);
	bool RegisterUserEmotes(const UserCharacterEmotes& emotes);
	bool RegisterUserWinLossStats(const UserWinLossStats& stats);
	bool RegisterUserBallons(const UserBallons& ballon);
	bool RegisterUserIcons(const UserIcons& icon);

	bool SaveUserAccount(const std::string& userId);
	bool SaveUserProfile(const std::string& userId);
	bool SaveUserCharacters(const std::string& userId);
	bool SaveUserEmotes(const std::string& userId);
	bool SaveUserWinLossStats(const std::string& userId);
	bool SaveUserBallons(const std::string& userId);
	bool SaveUserIcons(const std::string& userId);


	string CheckLogin(const string& id, const string& pw);
	string RegisterUser(const string& id, const string& pw, const string& nickname);
	void BroadcastLobbyUserList();
	void SendUserInfoByNickname(shared_ptr<ClientInfo> client, const string& nickname);
	void BroadcastLobbyChatMessage(const string& nickname, const string& message);
	void LogoutUser(shared_ptr<ClientInfo> client);

	vector<int> GetEmotionsByUserId(const std::string& userId);
	int GetBalloonByUserId(const std::string& userId);
	vector<int> GetCharactersByUserId(const std::string& userId);
	UserProfile& GetUserProfileById(const std::string& id);
	UserWinLossStats& GetUserWinLossStatsById(const std::string& id);
	UserCharacterEmotes& GetUserEmotesById(const std::string& userId);
	UserBallons& GetUserBallonsById(const std::string& userId);
	UserIcons& GetUserIconsById(const std::string& userId);
	UserCharacters& GetUserCharactersById(const std::string& userId);


	void UpdateWinLoss(const std::string& userId, bool isWin, int charIndex);
	bool LoadAttackStats(const std::string& path); // Startup only, before accepting clients.
	int GetAttackByIndex(int index);


private:
	Server& server_;
	ClientHandler* clientHandler_ = nullptr;
	//ClientHandler& clientHandler_;
	std::recursive_mutex usersMutex;
    CsvStore::Images pending_;
    bool transactionActive_ = false;
    std::string ReadCsv(const std::string& path);
    bool WriteCsv(const std::string& path, const std::string& data);
	std::unordered_map<int, int> attackByIndex_; // Immutable after startup.
	vector<UserAccount> users;
	vector<UserProfile> userProfiles;
	vector<UserCharacters> userCharacters;
	vector<UserCharacterEmotes> userEmotes;
	vector<UserWinLossStats> userStats;
	vector<UserBallons> userBallons;
	vector<UserIcons> userIcons;
};

