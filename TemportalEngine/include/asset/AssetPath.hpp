#pragma once

#include "TemportalEnginePCH.hpp"

#include "asset/AssetType.hpp"

NS_ASSET

/**
 * The identifier of an asset based on their registered `AssetType` and a path relative to the project's root directory.
 */
class AssetPath
{

private:
	AssetType mType;
	std::filesystem::path mPath;
	bool mbIsAbsolute;

public:
	AssetPath() = default;
	AssetPath(AssetType type, std::filesystem::path path, bool bIsAbsolute = false);

	operator bool() const { return this->isValid(); }
	bool isValid() const;
	std::string toString() const;
	std::string toShortName() const;
	std::filesystem::path toAbsolutePath() const;

	static AssetPath fromString(std::string fullStr)
	{
		auto delimiter = fullStr.find(':');
		return delimiter != std::string::npos
			? AssetPath(fullStr.substr(0, delimiter), fullStr.substr(delimiter + 1))
			: AssetPath();
	}

};

NS_END
