#pragma once
#include <unordered_map>
#include <string>

enum class PriceType
{
    COIN,
    CASH
};

struct StoreCharInfo
{
    int price;
    PriceType type;
};

struct StoreBalloonInfo
{
    int price;
    PriceType type;
};

struct StoreEmoteInfo
{
    int price;
    PriceType type;
};

struct StoreIconInfo
{
    int price;
    PriceType type;
};

class StoreData
{
public:
    static bool LoadStoreCharacters(const std::string& filename);
    static bool LoadStoreBalloons(const std::string& filename);
    static bool LoadStoreEmotes(const std::string& filename);
    static bool LoadStoreIcons(const std::string& filename);

    static bool GetStoreCharInfo(int charIndex, StoreCharInfo& outInfo);
    static bool GetStoreBalloonInfo(int balloonIndex, StoreBalloonInfo& outInfo);
    static bool GetStoreEmoteInfo(int emoteIndex, StoreEmoteInfo& outInfo);
    static bool GetStoreIconInfo(int iconIndex, StoreIconInfo& outInfo);

private:
    static std::unordered_map<int, StoreCharInfo> storeCharMap_;
    static std::unordered_map<int, StoreBalloonInfo> storeBalloonMap_;
    static std::unordered_map<int, StoreEmoteInfo> storeEmoteMap_;
    static std::unordered_map<int, StoreIconInfo> storeIconMap_;
};