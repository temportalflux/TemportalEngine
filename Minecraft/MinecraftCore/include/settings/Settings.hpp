#pragma once

#include "CoreInclude.hpp"

#include <cereal/access.hpp>
#include <cereal/archives/json.hpp>

NS_GAME

class Settings
{
	friend class cereal::access;

public:
	Settings() = default;
	Settings(std::filesystem::path const& filePath);

	void writeToDisk();
	void readFromDisk();

protected:
	virtual void save(cereal::JSONOutputArchive &archive) const = 0;
	virtual void load(cereal::JSONInputArchive &archive) = 0;

private:
	std::filesystem::path mFilePath;

};

NS_END
