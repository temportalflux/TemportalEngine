#pragma once

#include "CoreInclude.hpp"

NS_RESOURCE

class ResourceManager
{
	friend class PackManager;

private:
	ResourceManager() = default;

	ResourceManager& scan(std::filesystem::path const& path);

private:
	typedef std::unordered_map<std::string, std::filesystem::path> ResourceList;
	std::unordered_map<std::string, ResourceList> mResourceListByType;

};

class PackManager
{

public:
	PackManager() = default;

	void scanPacksIn(std::filesystem::path const& directory);

private:
	
	struct Pack
	{
		ResourceManager textures;
	};

	std::unordered_map<std::string, Pack> mPacks;

};

NS_END
