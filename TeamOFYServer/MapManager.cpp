#include "MapManager.h"

pair<vector<vector<int>>, vector<pair<int, int>>> MapManager::LoadMapByNameWithSpawns(const string& name)
{
    if (name != "Map1" && name != "Map2") return {};
    string filePath = name + ".csv";
    ifstream file(filePath);
    vector<vector<int>> mapData;
    vector<pair<int, int>> spawnPoints; // (x, y)

    if (!file.is_open()) {
        return { mapData, spawnPoints };
    }

    string line;
    int y = 0;
    while (getline(file, line)) {
        stringstream ss(line);
        string cell;
        vector<int> row;
        int x = 0;

        while (getline(ss, cell, ',')) {
            try {
                size_t used=0; int value = stoi(cell, &used);
                if (used!=cell.size() || value < -1 || value > (name=="Map1"?5:7)) return {};
                if (value == -1) {
                    spawnPoints.emplace_back(x, y);  // 층 정보 없이 전체 y 그대로 저장
                    row.push_back(5); // -1은 빈칸으로 처리
                }
                else {
                    row.push_back(value);
                }
            }
            catch (...) {
                return {};
            }
            x++;
        }

        if(row.size()!=15 || y>=26)return {};
        mapData.push_back(row);
        y++;
    }

    if(mapData.size()!=26)return {};
    return { mapData, spawnPoints };
}
