#pragma once
#include <string>
#include <cstdint>
#include <chrono>
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

    struct Combatant {
        int health = 0;
        int attack = 0;
        int character = 0;
        std::chrono::steady_clock::time_point nextDamage{};
    };
    unordered_map<string, Combatant> combat;
    uint64_t combatRevision = 0;
    bool rewardsPending = false;
    bool rewardsCommitted = false;
    uint64_t matchId = 0;
    bool isInGame = false;
    bool isCoopMode = false; // true면 2VS2 협동전, false면 개인전
};