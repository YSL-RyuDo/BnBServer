#pragma once
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>
#include <string>
#include <tuple>
#include "Utils.h"

class MapManager
{
public:
	pair<vector<vector<int>>, vector<pair<int, int>>> LoadMapByNameWithSpawns(const string& name);
};

