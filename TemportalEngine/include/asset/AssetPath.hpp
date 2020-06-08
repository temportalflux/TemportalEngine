#pragma once

#include "TemportalEnginePCH.hpp"

#include "Engine.hpp"
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
	AssetPath(AssetType type, std::filesystem::path path, bool bIsAbsolute=false) : mType(type), mPath(path), mbIsAbsolute(bIsAbsolute) {}

	std::string toString() const { return this->mType + ":" + this->mPath.string(); }
	std::string toShortName() const { return this->mType + ":" + this->mPath.stem().string(); }

	std::filesystem::path toAbsolutePath() const
	{
		if (this->mbIsAbsolute) return this->mPath;
		assert(engine::Engine::Get()->hasProject());
		return std::filesystem::absolute(engine::Engine::Get()->getProject()->getAbsoluteDirectoryPath() / this->mPath);
	}

	template <typename Archive>
	void save(Archive &archive) const
	{
		archive(
			cereal::make_nvp("type", this->mType),
			cereal::make_nvp("path", this->mRelativePath.string())
		);
	}

	template <typename Archive>
	void load(Archive &archive)
	{
		std::string pathStr;
		archive(
			cereal::make_nvp("type", this->mType),
			cereal::make_nvp("path", pathStr)
		);
		this->mRelativePath = pathStr;
	}

};

NS_END
