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
    bool isCoopMode = false; // true면 2VS2 협동전, false면 개인전
};