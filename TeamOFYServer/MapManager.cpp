#include "MapManager.h"

pair<vector<vector<int>>, vector<pair<int, int>>> MapManager::LoadMapByNameWithSpawns(const string& name)
{
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
                int value = stoi(cell);
                if (value == -1) {
                    spawnPoints.emplace_back(x, y);  // �� ���� ���� ��ü y �״�� ����
                    row.push_back(5); // -1�� ��ĭ���� ó��
                }
                else {
                    row.push_back(value);
                }
            }
            catch (...) {
                row.push_back(0);
            }
            x++;
        }

        mapData.push_back(row);
        y++;
    }

    return { mapData, spawnPoints };
}
