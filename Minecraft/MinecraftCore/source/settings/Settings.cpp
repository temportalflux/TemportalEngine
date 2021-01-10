#include "settings/Settings.hpp"

#include "asset/Asset.hpp"
#include <iostream>

using namespace game;

Settings::Settings(std::filesystem::path const& filePath)
	: mFilePath(filePath)
{
}

void Settings::writeToDisk()
{
	std::filesystem::create_directories(this->mFilePath.parent_path());
	std::ofstream os(this->mFilePath);
	cereal::JSONOutputArchive archive(os, asset::Asset::JsonFormat);
	this->save(archive);
}

void Settings::readFromDisk()
{
	if (!std::filesystem::exists(this->mFilePath))
	{
		this->writeToDisk();
	}
	else
	{
		std::ifstream is(this->mFilePath);
		cereal::JSONInputArchive archive(is);
		this->load(archive);
	}
}
