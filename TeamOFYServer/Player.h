#pragma once
#include <unordered_map>
#include <mutex>
#include <string>
#include <cstring>
#include <iostream>
#include <vector>
#include <algorithm>
#include "Utils.h"

class Player
{
public:
	Player() = default;  // �⺻ ������ public���� ����
	Player(const Player&) = delete;
	Player& operator=(const Player&) = delete;

	bool UpdatePlayerPosition(const std::string& username, float x, float z);
	std::pair<float, float> GetPlayerPosition(const std::string& username);

	unordered_map<string, pair<float, float>> playerPositions;
	mutex playerPositionsMutex;
};

