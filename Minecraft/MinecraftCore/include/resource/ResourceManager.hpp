#pragma once

#include "CoreInclude.hpp"

NS_RESOURCE

class ResourceManager
{
	friend class Pack;

public:
	ResourceManager() = default;

private:
	typedef std::unordered_map<std::string, std::filesystem::path> ResourceList;
	std::unordered_map<std::string, ResourceList> mResourceListByType;

	ResourceManager& scan(std::filesystem::path const& path);
	virtual void load() = 0;

};

class TextureManager : public ResourceManager
{
	friend class Pack;
private:
	TextureManager() : ResourceManager() {}
	void load() override;
};

class Pack
{
	friend class PackManager;

public:
	Pack(std::filesystem::path const& path);

private:
	std::filesystem::path mRootPath;
	TextureManager mTextures;

	Pack& scan();
	void load();

};

class PackManager
{

public:
	PackManager() = default;

	void scanPacksIn(std::filesystem::path const& directory);
	bool hasPack(std::string const& packName) const;
	void loadPack(std::string const& packName);

private:
	std::unordered_map<std::string, Pack> mPacks;

};

NS_END
