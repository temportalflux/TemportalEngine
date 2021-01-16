#pragma once

#include "CoreInclude.hpp"

#include "cereal/cerealCore.hpp"

NS_GAME

class JsonData
{
	friend class cereal::access;

public:
	JsonData() = default;
	JsonData(std::filesystem::path const& filePath);

	void writeToDisk();
	void readFromDisk();

protected:
	virtual void save(cereal::JSONOutputArchive &archive) const = 0;
	virtual void load(cereal::JSONInputArchive &archive) = 0;

private:
	std::filesystem::path mFilePath;

};

NS_END
