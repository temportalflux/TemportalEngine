#pragma once

#include "CoreInclude.hpp"

NS_RESOURCE

class ResourceManager
{
	friend class Pack;

public:
	ResourceManager() = default;

	std::set<std::string> getEntryKeysForType(std::string const& type) const;

protected:
	typedef std::unordered_map<std::string, std::filesystem::path> ResourceList;
	std::unordered_map<std::string, ResourceList> mResourceListByType;

	ResourceManager& scan(std::filesystem::path const& path);
	virtual void load() = 0;
	virtual void unload() = 0;

};

class TextureManager : public ResourceManager
{
	friend class Pack;

public:

	struct TextureData
	{
		math::Vector2UInt size;
		std::vector<ui8> pixels;
	};

	TextureData const& getEntry(std::string const& type, std::string const& key) const;

private:
	TextureManager() : ResourceManager() {}

	std::unordered_map</*std::filesystem::path*/ std::string, TextureData> mTextures;

	void load() override;
	void loadTexture(std::filesystem::path const& path, TextureData &outData) const;
	void unload() override;

};

class Pack
{
	friend class PackManager;

public:
	Pack(std::filesystem::path const& path);

private:
	std::filesystem::path mRootPath;
	TextureManager mTextures;
	bool mbIsLoaded;

	Pack& scan();
	void load();
	void unload();

};

class PackManager
{

public:
	PackManager() = default;

	BroadcastDelegate<void(PackManager*)> OnResourcesLoadedEvent;

	void scanPacksIn(std::filesystem::path const& directory);
	bool hasPack(std::string const& packName) const;
	PackManager& loadPack(std::string const& packName, ui8 const& priority);
	PackManager& unloadPack(std::string const& packName);
	void commitChanges();

	struct PackEntry
	{
		std::string packName;
		std::string type;
		std::string entryKey;
		std::string textureId() const { return type + ":" + entryKey; }
	};

	std::vector<PackEntry> getTexturesOfType(std::string const& type);
	TextureManager::TextureData const& getTextureData(PackEntry const& entry);

private:
	std::unordered_map<std::string, Pack> mPacks;
	std::map<ui8, std::string> mPackPriority;

};

NS_END
