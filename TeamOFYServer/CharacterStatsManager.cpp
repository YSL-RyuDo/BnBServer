#include "CharacterStatsManager.h"
#include <fstream>
#include <sstream>
#include <iostream>

bool CharacterStatsManager::LoadFromName(const std::string& name) {
    return LoadFromCSV(name + ".csv");
}

bool CharacterStatsManager::LoadFromCSV(const std::string& csvFilePath)
{
    std::ifstream file(csvFilePath);
    if (!file.is_open())
    {
        std::cerr << "CSV 파일 열기 실패: " << csvFilePath << std::endl;
        return false;
    }

    charStatsMap_.clear();

    std::string line;
    // 헤더 라인 건너뛰기
    std::getline(file, line);

    while (std::getline(file, line))
    {
        std::stringstream ss(line);
        std::string token;

        int charIndex = -1;
        CharStats stats;

        // CharIndex
        if (!std::getline(ss, token, ','))
            continue;
        charIndex = std::stoi(token);

        // Health
        if (!std::getline(ss, token, ','))
            continue;
        stats.health = std::stoi(token);

        // Attack
        if (!std::getline(ss, token, ','))
            continue;
        stats.attack = std::stoi(token);

        charStatsMap_[charIndex] = stats;
    }

    return true;
}

const CharStats* CharacterStatsManager::GetStats(int charIndex) const
{
    auto it = charStatsMap_.find(charIndex);
    if (it != charStatsMap_.end())
    {
        return &(it->second);
    }
    return nullptr;
}
