#include "resource/ResourceManager.hpp"

using namespace resource;

void PackManager::scanPacksIn(std::filesystem::path const& directory)
{
	for (auto const& entry : std::filesystem::directory_iterator(directory))
	{
		this->mPacks.insert(std::make_pair(
			entry.path().stem().string(),
			std::move(Pack(entry.path()).scan())
		));
	}
}

Pack::Pack(std::filesystem::path const& path) : mRootPath(path), mTextures()
{
}

Pack& Pack::scan()
{
	this->mTextures.scan(this->mRootPath / "texture");
	return *this;
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

bool PackManager::hasPack(std::string const& packName) const
{
	return this->mPacks.find(packName) != this->mPacks.end();
}

void PackManager::loadPack(std::string const& packName)
{
	this->mPacks.find(packName)->second.load();
}

void Pack::load()
{
	this->mTextures.load();
}

void TextureManager::load()
{

}
