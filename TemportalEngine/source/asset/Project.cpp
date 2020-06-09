#include "asset/Project.hpp"

#include "asset/AssetManager.hpp"
#include <cereal/archives/json.hpp>
#include <cereal/archives/portable_binary.hpp>
#include "..\..\include\asset\Project.hpp"

using namespace asset;

DEFINE_NEWASSET_FACTORY(Project)
DEFINE_EMPTYASSET_FACTORY(Project)

Project::Project(std::filesystem::path filePath) : Asset(filePath)
{
	this->mName = this->getFileName();
}

Project::Project(std::string name, Version version) : Asset()
{
	this->mName = name;
	this->mVersion = version;
}

std::filesystem::path Project::getAbsoluteDirectoryPath() const
{
	return this->mFilePath.parent_path();
}

std::filesystem::path Project::getAssetDirectoryFor(std::filesystem::path projectDir)
{
	return projectDir / "assets";
}

std::filesystem::path Project::getAssetDirectory() const
{
	return Project::getAssetDirectoryFor(this->getAbsoluteDirectoryPath());
}

#pragma region Properties

std::string Project::getName() const
{
	return this->mName;
}

void Project::setName(std::string value)
{
	this->mName = value;
}

Version Project::getVersion() const
{
	return this->mVersion;
}

void Project::setVersion(Version value)
{
	this->mVersion = value;
}

std::string Project::getDisplayName() const
{
	return this->getName() + " (" + this->getVersion().toString() + ")";
}

graphics::PhysicalDevicePreference Project::getPhysicalDevicePreference() const
{
	return this->mGraphicsDevicePreference;
}

void Project::setPhysicalDevicePreference(graphics::PhysicalDevicePreference const &prefs)
{
	this->mGraphicsDevicePreference = prefs;
}

#pragma endregion

CREATE_DEFAULT_SERIALIZATION_DEFINITION(const, Project::write, cereal::JSONOutputArchive, Project::serialize);
CREATE_DEFAULT_SERIALIZATION_DEFINITION(, Project::read, cereal::JSONInputArchive, Project::deserialize);
CREATE_DEFAULT_SERIALIZATION_DEFINITION(const, Project::compile, cereal::PortableBinaryOutputArchive, Project::serialize);
CREATE_DEFAULT_SERIALIZATION_DEFINITION(, Project::decompile, cereal::PortableBinaryInputArchive, Project::deserialize);
