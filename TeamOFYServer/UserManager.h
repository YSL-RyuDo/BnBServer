#pragma once
#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>
#include <mutex>
#include "UserAccount.h"
#include "Utils.h"
#include "ClientInfo.h"

class Server;
class ClientHandler;

class UserManager
{
public:
	UserManager(Server& server);
	void SetClientHandler(ClientHandler* handler) { clientHandler_ = handler; }

	vector<UserAccount> LoadAccountUsers(const string&);
	vector<UserProfile> LoadUserProfiles(const string&);
	vector<UserCharacters> LoadUserCharacters(const string&);
	vector<UserCharacterEmotes> LoadUserCharacterEmotes(const string&);
	vector<UserWinLossStats> LoadUserWinLossStats(const string&);

	void SaveUsers(const vector<UserAccount>&, const string&);
	string CheckLogin(const string& id, const string& pw);
	string RegisterUser(const string& id, const string& pw, const string& nickname);
	void BroadcastLobbyUserList();
	void SendUserInfoByNickname(shared_ptr<ClientInfo> client, const string& nickname);
	void BroadcastLobbyChatMessage(const string& nickname, const string& message);
	void LogoutUser(shared_ptr<ClientInfo> client);
	vector<int> GetEmotionsByUserId(const std::string& userId);
private:
	Server& server_;
	ClientHandler* clientHandler_ = nullptr;
	//ClientHandler& clientHandler_;
	mutex usersMutex;
	vector<UserAccount> users;
	vector<UserProfile> userProfiles;
	vector<UserCharacters> userCharacters;
	vector<UserCharacterEmotes> userEmotes;
	vector<UserWinLossStats> userStats;
};

