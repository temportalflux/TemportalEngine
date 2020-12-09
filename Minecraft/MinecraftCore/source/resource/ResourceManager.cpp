#include "resource/ResourceManager.hpp"

#include "utility/ImageUtils.hpp"

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
	this->mTextures.scan(this->mRootPath / "textures");
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

PackManager& PackManager::loadPack(std::string const& packName, ui8 const& priority)
{
	this->mPacks.find(packName)->second.load();
	auto iterFirstNotLessThan = std::lower_bound(
		this->mPackPriority.cbegin(), this->mPackPriority.cend(),
		priority, [](auto const& entry, ui8 const& pri) { return entry.first < pri; }
	);
	this->mPackPriority.insert(iterFirstNotLessThan, std::make_pair(priority, packName));
	return *this;
}

void Pack::load()
{
	this->mTextures.load();
	this->mbIsLoaded = true;
}

void TextureManager::load()
{
	for (auto const& [type, entryList] : this->mResourceListByType)
	{
		for (auto const& [id, path] : entryList)
		{
			TextureData data = {};
			this->loadTexture(path, data);
			this->mTextures.insert(std::make_pair(path.string(), std::move(data)));
		}
	}
}

void TextureManager::loadTexture(std::filesystem::path const& path, TextureData &outData) const
{
	static i32 LOAD_MODE = STBI_rgb_alpha;
	static ui32 LOAD_MODE_SIZE = 4; // 4 bytes per pixel
	i32 width, height, srcChannels;

	ui8* data = stbi_load(path.string().c_str(), &width, &height, &srcChannels, LOAD_MODE);
	if (data == nullptr) { return; }
	
	outData.size = { (ui32)width, (ui32)height };
	outData.pixels.resize(outData.size.x() * outData.size.y() * LOAD_MODE_SIZE);

	memcpy(outData.pixels.data(), data, outData.pixels.size());
	stbi_image_free(data);
}

PackManager& PackManager::unloadPack(std::string const& packName)
{
	auto& pack = this->mPacks.find(packName)->second;
	pack.unload();
	// std::map erase-remove-if
	for (auto iter = this->mPackPriority.begin(); iter != this->mPackPriority.end();)
	{
		if (iter->second == packName) iter = this->mPackPriority.erase(iter);
		else ++iter;
	}
	return *this;
}

void Pack::unload()
{
	this->mTextures.unload();
	this->mbIsLoaded = false;
}

void TextureManager::unload()
{
	this->mTextures.clear();
}

void PackManager::commitChanges()
{
	OPTICK_EVENT()
	this->OnResourcesLoadedEvent.broadcast(this);
}

std::vector<PackManager::PackEntry> PackManager::getTexturesOfType(std::string const& type)
{
	auto uniqueKeys = std::set<std::string>();
	auto entries = std::vector<PackEntry>();
	auto begin = std::crbegin(this->mPackPriority);
	auto end = std::crend(this->mPackPriority);
	for (auto iter = begin; iter != end; ++iter)
	{
		auto const& packIter = this->mPacks.find(iter->second);
		auto const& pack = packIter->second;
		for (auto const& key : pack.mTextures.getEntryKeysForType(type))
		{
			if (uniqueKeys.find(key) == uniqueKeys.end())
			{
				uniqueKeys.insert(key);
				entries.push_back(PackEntry {
					packIter->first, type, key
				});
			}
		}
	}
	return entries;
}

std::set<std::string> ResourceManager::getEntryKeysForType(std::string const& type) const
{
	auto uniqueKeys = std::set<std::string>();
	auto resourceListIter = this->mResourceListByType.find(type);
	if (resourceListIter == this->mResourceListByType.end()) return uniqueKeys;
	for (auto const& key : resourceListIter->second) { uniqueKeys.insert(key.first); }
	return uniqueKeys;
}

TextureManager::TextureData const& PackManager::getTextureData(PackEntry const& entry)
{
	auto const& pack = this->mPacks.find(entry.packName)->second;
	return pack.mTextures.getEntry(entry.type, entry.entryKey);
}

TextureManager::TextureData const& TextureManager::getEntry(std::string const& type, std::string const& key) const
{
	auto const& resourceList = this->mResourceListByType.find(type)->second;
	auto const& entryPath = resourceList.find(key)->second;
	return this->mTextures.find(entryPath.string())->second;
}
