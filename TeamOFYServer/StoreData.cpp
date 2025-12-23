#include "StoreData.h"
#include <fstream>
#include <sstream>
#include <iostream>

std::unordered_map<int, StoreCharInfo> StoreData::storeCharMap_;
std::unordered_map<int, StoreBalloonInfo> StoreData::storeBalloonMap_;
std::unordered_map<int, StoreEmoteInfo> StoreData::storeEmoteMap_;
std::unordered_map<int, StoreIconInfo> StoreData::storeIconMap_;

static PriceType ParsePriceType(const std::string& str)
{
    if (str == "COIN") return PriceType::COIN;
    if (str == "CASH")  return PriceType::CASH;
    return PriceType::COIN;
}

bool StoreData::LoadStoreCharacters(const std::string& filename)
{
    std::ifstream file(filename);
    if (!file.is_open())
    {
        std::cerr << "[StoreData] CSV 열기 실패: " << filename << std::endl;
        return false;
    }

    storeCharMap_.clear();

    std::string line;
    std::getline(file, line); // 헤더 스킵

    while (std::getline(file, line))
    {
        if (line.empty()) continue;

        std::stringstream ss(line);
        std::string idxStr, priceStr, typeStr;

        if (!std::getline(ss, idxStr, ',')) continue;
        if (!std::getline(ss, priceStr, ',')) continue;
        if (!std::getline(ss, typeStr, ',')) continue;

        int charIndex = std::stoi(idxStr);
        int price = std::stoi(priceStr);
        PriceType type = ParsePriceType(typeStr);

        storeCharMap_[charIndex] = { price, type };
    }

    std::cout << "[StoreData] 캐릭터 상점 로드 완료\n";
    return true;
}

bool StoreData::LoadStoreBalloons(const std::string& filename)
{
    std::ifstream file(filename);
    if (!file.is_open())
    {
        std::cerr << "[StoreData] StoreBalloon.csv 열기 실패\n";
        return false;
    }

    storeBalloonMap_.clear();

    std::string line;
    std::getline(file, line); // 헤더 스킵

    while (std::getline(file, line))
    {
        if (line.empty()) continue;

        std::stringstream ss(line);
        std::string idxStr, priceStr, typeStr;

        std::getline(ss, idxStr, ',');
        std::getline(ss, priceStr, ',');
        std::getline(ss, typeStr, ',');

        int idx = std::stoi(idxStr);
        int price = std::stoi(priceStr);
        PriceType type = (typeStr == "COIN") ? PriceType::COIN : PriceType::CASH;

        storeBalloonMap_[idx] = { price, type };
    }

    std::cout << "[StoreData] 물풍선 상점 로드 완료\n";
    return true;
}

bool StoreData::LoadStoreEmotes(const std::string& filename)
{
    std::ifstream file(filename);
    if (!file.is_open())
    {
        std::cerr << "[StoreData] StoreEmotes.csv 열기 실패\n";
        return false;
    }

    storeEmoteMap_.clear();

    std::string line;
    std::getline(file, line); // 헤더 스킵

    while (std::getline(file, line))
    {
        if (line.empty()) continue;

        std::stringstream ss(line);
        std::string idxStr, priceStr, typeStr;

        std::getline(ss, idxStr, ',');
        std::getline(ss, priceStr, ',');
        std::getline(ss, typeStr, ',');

        int emoteIndex = std::stoi(idxStr);
        int price = std::stoi(priceStr);
        PriceType type = ParsePriceType(typeStr);

        storeEmoteMap_[emoteIndex] = { price, type };
    }

    std::cout << "[StoreData] 이모티콘 상점 로드 완료 ("
        << storeEmoteMap_.size() << "개)\n";
    return true;
}

bool StoreData::LoadStoreIcons(const std::string& filename)
{
    std::ifstream file(filename);
    if (!file.is_open())
    {
        std::cerr << "[StoreData] StoreIcons.csv 열기 실패\n";
        return false;
    }

    storeIconMap_.clear();

    std::string line;
    std::getline(file, line); // 헤더 스킵

    while (std::getline(file, line))
    {
        if (line.empty()) continue;

        std::stringstream ss(line);
        std::string idxStr, priceStr, typeStr;

        std::getline(ss, idxStr, ',');
        std::getline(ss, priceStr, ',');
        std::getline(ss, typeStr, ',');

        int idx = std::stoi(idxStr);
        int price = std::stoi(priceStr);
        PriceType type = ParsePriceType(typeStr);

        storeIconMap_[idx] = { price, type };
    }

    std::cout << "[StoreData] 아이콘 상점 로드 완료 ("
        << storeIconMap_.size() << "개)\n";
    return true;
}


bool StoreData::GetStoreCharInfo(int charIndex, StoreCharInfo& outInfo)
{
    auto it = storeCharMap_.find(charIndex);
    if (it == storeCharMap_.end())
        return false;

    outInfo = it->second;
    return true;
}

bool StoreData::GetStoreBalloonInfo(int balloonIndex, StoreBalloonInfo& outInfo)
{
    auto it = storeBalloonMap_.find(balloonIndex);
    if (it == storeBalloonMap_.end())
        return false;

    outInfo = it->second;
    return true;
}

bool StoreData::GetStoreEmoteInfo(int emoteIndex, StoreEmoteInfo& outInfo)
{
    auto it = storeEmoteMap_.find(emoteIndex);
    if (it == storeEmoteMap_.end())
        return false;

    outInfo = it->second;
    return true;
}

bool StoreData::GetStoreIconInfo(int iconIndex, StoreIconInfo& outInfo)
{
    auto it = storeIconMap_.find(iconIndex);
    if (it == storeIconMap_.end())
        return false;

    outInfo = it->second;
    return true;
}
