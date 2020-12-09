#include "resource/ResourceManager.hpp"

using namespace resource;

void PackManager::scanPacksIn(std::filesystem::path const& directory)
{
	for (auto const& entry : std::filesystem::directory_iterator(directory))
	{
		this->mPacks.insert(std::make_pair(
			entry.path().stem().string(),
			Pack {
				ResourceManager().scan(entry.path() / "textures")
			}
		));
	}
}

ResourceManager& ResourceManager::scan(std::filesystem::path const& path)
{
	if (std::filesystem::exists(path))
	{
		for (auto const& entry : std::filesystem::directory_iterator(path))
		{
			auto resourceList = ResourceList();
			for (auto const& resourceEntry : std::filesystem::directory_iterator(entry.path()))
			{
				resourceList.insert(std::make_pair(resourceEntry.path().stem().string(), resourceEntry.path()));
			}
			this->mResourceListByType.insert(std::make_pair(entry.path().stem().string(), std::move(resourceList)));
		}
	}
	return *this;
}

