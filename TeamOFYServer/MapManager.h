#pragma once
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>
#include <string>
#include "Utils.h"

class MapManager
{
public:
	vector<vector<int>> LoadMapByName(const string& name);
};

