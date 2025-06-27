#pragma once
#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>
#include "User.h"
#include "Utils.h"
class UserManager
{
public:
	vector<User> LoadUsers(const string& filename);
	void SaveUsers(const vector<User>& users, const string& filename);

private:
	vector<User> users;
};

