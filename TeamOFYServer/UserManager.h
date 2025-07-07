#pragma once
#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>
#include <mutex>
#include "User.h"
#include "Utils.h"
#include "ClientInfo.h"

class Server;
class ClientHandler;

class UserManager
{
public:
	UserManager(Server& server);
	void SetClientHandler(ClientHandler* handler) { clientHandler_ = handler; }
	vector<User> LoadUsers(const string&);
	void SaveUsers(const vector<User>&, const string&);
	string CheckLogin(const string& id, const string& pw);
	string RegisterUser(const string& id, const string& pw, const string& nickname);
	void BroadcastLobbyUserList();
	void SendUserInfoByNickname(shared_ptr<ClientInfo> client, const string& nickname);
	void BroadcastLobbyChatMessage(const string& nickname, const string& message);
	void LogoutUser(shared_ptr<ClientInfo> client);

private:
	Server& server_;
	ClientHandler* clientHandler_ = nullptr;
	//ClientHandler& clientHandler_;
	mutex usersMutex;
	vector<User> users;
};

