#pragma once
#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>
#include <mutex>
#include "User.h"
#include "Utils.h"
class UserManager
{
public:
	vector<User> LoadUsers(const string&);
	void SaveUsers(const vector<User>&, const string&);
	string CheckLogin(const string& id, const string& pw);
	string RegisterUser(const string& id, const string& pw, const string& nickname);
	
private:
	mutex usersMutex;
	vector<User> users;
};

