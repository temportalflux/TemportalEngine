#pragma once

#include "TemportalEnginePCH.hpp"

#include "logging/Logger.hpp"

FORWARD_DEF(NS_ASSET, class Asset);

NS_BUILD

class BuildAsset
{

public:
	static std::shared_ptr<BuildAsset> create(std::shared_ptr<asset::Asset> asset);

	BuildAsset() = default;
	BuildAsset(std::shared_ptr<asset::Asset> asset);

	template <typename TAsset>
	std::shared_ptr<TAsset> get() const
	{
		return std::dynamic_pointer_cast<TAsset>(this->mpAsset);
	}

	void setOutputPath(std::filesystem::path const &path);
	virtual std::vector<std::string> compile(logging::Logger &logger);
	virtual void save();

protected:
	std::shared_ptr<asset::Asset> getAsset() const;

private:
	std::filesystem::path mOutputPath;
	std::shared_ptr<asset::Asset> mpAsset;

};

NS_END
