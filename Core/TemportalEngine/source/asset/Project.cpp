#include "asset/Project.hpp"

#include "asset/AssetManager.hpp"
#include <cereal/archives/json.hpp>
#include <cereal/archives/portable_binary.hpp>

using namespace asset;

DEFINE_PROPERTY_CONTAINER(Project)
DEFINE_FACTORY_ASSET_METADATA(Project)

Project::Project() : Asset()
{
	this->mGraphicsDevicePreference
		.addCriteriaQueueFamily(graphics::EQueueFamily::eGraphics)
		.addCriteriaQueueFamily(graphics::EQueueFamily::ePresentation)
		.addCriteriaDeviceExtension(graphics::PhysicalDeviceExtension::ExtSwapChain);
}

Project::Project(std::filesystem::path filePath) : Asset(filePath)
{
	this->mName = this->getFileName();
}

Project::Project(std::string name, Version version) : Project()
{
	this->mName = name;
	this->mVersion = version;
}

std::vector<AssetPath const*> Project::getAssetRefs() const { return { &mRenderPass.path() }; }
std::vector<AssetPath*> Project::getAssetRefs() { return { &mRenderPass.path() }; }

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

std::string Project::getDisplayName() const
{
	return this->getName() + " (" + this->getVersion().toString() + ")";
}

graphics::LogicalDeviceInfo Project::getGraphicsDeviceInitInfo() const
{
	auto info = graphics::LogicalDeviceInfo();
	for (const auto& prefExtension : this->mGraphicsDevicePreference.getDeviceExtensions())
	{
		if (!prefExtension.isRequired()) continue;
		info.addDeviceExtension(prefExtension.value);
	}
	for (const auto& prefQueueFamily : this->mGraphicsDevicePreference.getQueueFamilies())
	{
		if (!prefQueueFamily.isRequired()) continue;
		info.addQueueFamily(prefQueueFamily.value);
	}
	return info;
}

CREATE_DEFAULT_SERIALIZATION_DEFINITION(const, Project::write, cereal::JSONOutputArchive, Project::serialize);
CREATE_DEFAULT_SERIALIZATION_DEFINITION(, Project::read, cereal::JSONInputArchive, Project::deserialize);
CREATE_DEFAULT_SERIALIZATION_DEFINITION(const, Project::compile, cereal::PortableBinaryOutputArchive, Project::serialize);
CREATE_DEFAULT_SERIALIZATION_DEFINITION(, Project::decompile, cereal::PortableBinaryInputArchive, Project::deserialize);
