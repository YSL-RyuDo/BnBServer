#pragma once

#include <string>
#include <unordered_map>

struct CharStats {
    int health = 0;
    int attack = 0;
};

class CharacterStatsManager
{
public:
    // CSV 파일에서 캐릭터 스탯을 불러옴
    bool LoadFromName(const std::string& name);
    bool LoadFromCSV(const std::string& csvFilePath);

    // 캐릭터 인덱스에 해당하는 스탯을 반환, 없으면 nullptr 반환
    const CharStats* GetStats(int charIndex) const;

private:
    std::unordered_map<int, CharStats> charStatsMap_;
};
