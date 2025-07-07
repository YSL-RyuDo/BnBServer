#pragma once
#include <string>
#include <vector>

using namespace std;

struct Room {
    string roomName;
    string mapName;
    string password;
    vector<string> users;
    vector<pair<int, int>> assignedSpawnPositions;
};