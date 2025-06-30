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
	vector<User> LoadUsers(const string&);
	void SaveUsers(const vector<User>&, const string&);

private:
	vector<User> users;
};

