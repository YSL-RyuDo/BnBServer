#pragma once
#include <string>
#include <vector>
#include <unordered_map>

using namespace std;

struct Room {
    string roomName;
    string mapName;
    string password;
    vector<string> users;
    vector<pair<int, int>> assignedSpawnPositions;
    unordered_map<string, int> characterSelections; 

    bool isInGame = false;
};