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
	std::vector<User> LoadUsers(const std::string& filename);
	void SaveUsers(const std::vector<User>& users, const std::string& filename);

private:
	std::vector<User> users;
};

