#pragma once

#include "TemportalEnginePCH.hpp"

#include <string>

NS_ASSET

class AssetManager
{

public:
	static void createProject(std::string filePath, std::string name);
	static void openProject(std::string filePath);
	
	void queryAssetTypes();

};

NS_END
