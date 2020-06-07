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
	std::filesystem::path mRelativePath;

public:
	AssetPath(AssetType type, std::filesystem::path path) : mType(type), mRelativePath(path) {}

	std::string toString() const { return this->mType + ":" + this->mRelativePath.string(); }
	std::string toShortName() const { return this->mType + ":" + this->mRelativePath.stem().string(); }

	std::filesystem::path toAbsolutePath() const
	{
		assert(engine::Engine::Get()->hasProject());
		return std::filesystem::absolute(engine::Engine::Get()->getProject()->getAbsoluteDirectoryPath() / this->mRelativePath);
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
