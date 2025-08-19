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
    unordered_map<string, string> teamAssignments; // <����ID, �� �̸�>

    bool isInGame = false;
    bool isCoopMode = false; // true�� 2VS2 ������, false�� ������
};