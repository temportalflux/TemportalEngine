#include "asset/Project.hpp"

#include "asset/AssetManager.hpp"
#include <cereal/archives/json.hpp>
#include <cereal/archives/portable_binary.hpp>

using namespace asset;

Project::Project(std::string name, Version version) : Asset()
{
	this->mName = name;
	this->mVersion = version;
}

std::string Project::getName() const
{
	return this->mName;
}

Version Project::getVersion() const
{
	return this->mVersion;
}

std::string Project::getDisplayName() const
{
	return this->getName() + " (" + this->getVersion().toString() + ")";
}

std::filesystem::path Project::getAbsoluteDirectoryPath() const
{
	return this->mProjectDirectory;
}

std::filesystem::path Project::getAssetDirectoryFor(std::filesystem::path projectDir)
{
	return projectDir / "assets";
}

std::filesystem::path Project::getAssetDirectory() const
{
	return Project::getAssetDirectoryFor(this->getAbsoluteDirectoryPath());
}

std::shared_ptr<Asset> Project::createAsset(std::filesystem::path filePath)
{
	auto ptr = asset::AssetManager::makeAsset<Project>();
	ptr->mName = filePath.stem().string();
	ptr->mProjectDirectory = filePath.parent_path();
	ptr->writeToDisk(filePath, EAssetSerialization::Json);
	return ptr;
}

std::shared_ptr<Asset> Project::readFromDisk(std::filesystem::path filePath, EAssetSerialization type)
{
	auto ptr = asset::AssetManager::makeAsset<Project>();
	switch (type)
	{
	case EAssetSerialization::Json:
	{
		std::ifstream is(filePath);
		cereal::JSONInputArchive archive(is);
		ptr->load(archive);
		break;
	}
	case EAssetSerialization::Binary:
	{
		std::ifstream is(filePath, std::ios::binary);
		cereal::PortableBinaryInputArchive archive(is);
		ptr->load(archive);
		break;
	}
	}
	ptr->mProjectDirectory = filePath.parent_path();
	return ptr;
}

void Project::writeToDisk(std::filesystem::path filePath, EAssetSerialization type) const
{
	switch (type)
	{
	case EAssetSerialization::Json:
	{
		std::ofstream os(filePath);
		cereal::JSONOutputArchive archive(os);
		this->save(archive);
		return;
	}
	case EAssetSerialization::Binary:
	{
		std::ofstream os(filePath, std::ios::trunc | std::ios::binary);
		cereal::PortableBinaryOutputArchive archive(os);
		this->save(archive);
		return;
	}
	}
}
