#include "MapManager.h"

vector<vector<int>> MapManager::LoadMapByName(const string& name)
{
    string filePath = name + ".csv";  // ¿¹: "Map1.csv"
    ifstream file(filePath);
    vector<vector<int>> mapData;

    if (!file.is_open()) {
        cerr << "[MapManager] Failed to open file: " << filePath << endl;
        return mapData;
    }

    string line;
    while (getline(file, line)) {
        stringstream ss(line);
        string cell;
        vector<int> row;

        while (getline(ss, cell, ',')) {
            try {
                row.push_back(stoi(cell));
            }
            catch (...) {
                row.push_back(0);
            }
        }

        mapData.push_back(row);
    }

    file.close();
    return mapData;
}
