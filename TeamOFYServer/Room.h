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
    unordered_map<string, string> teamAssignments; // <유저ID, 팀 이름>

    bool isInGame = false;
    bool isCoopMode = false; // true면 2VS2 협동전, false면 개인전
};