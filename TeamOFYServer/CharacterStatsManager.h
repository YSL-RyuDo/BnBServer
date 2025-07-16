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
    // CSV ���Ͽ��� ĳ���� ������ �ҷ���
    bool LoadFromName(const std::string& name);
    bool LoadFromCSV(const std::string& csvFilePath);

    // ĳ���� �ε����� �ش��ϴ� ������ ��ȯ, ������ nullptr ��ȯ
    const CharStats* GetStats(int charIndex) const;

private:
    std::unordered_map<int, CharStats> charStatsMap_;
};
