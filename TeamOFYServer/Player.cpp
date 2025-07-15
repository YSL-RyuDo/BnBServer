#include "Player.h"

bool Player::UpdatePlayerPosition(const std::string& username, float x, float z)
{
    std::lock_guard<std::mutex> lock(playerPositionsMutex);
    playerPositions[username] = std::make_pair(x, z);
    return true; // 필요시 다른 로직 추가
}

std::pair<float, float> Player::GetPlayerPosition(const std::string& username)
{
    std::lock_guard<std::mutex> lock(playerPositionsMutex);
    auto it = playerPositions.find(username);
    if (it != playerPositions.end())
        return it->second;
    return { 0.f, 0.f }; // 기본값
}