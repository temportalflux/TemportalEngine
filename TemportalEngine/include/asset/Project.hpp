#pragma once

#include "asset/Asset.hpp"

#include "graphics/PhysicalDevicePreference.hpp"
#include "graphics/LogicalDeviceInfo.hpp"
#include "utility/Version.hpp"

FORWARD_DEF(NS_ASSET, class RenderPass)

NS_ASSET

class Project : public Asset
{
public:
	DECLARE_PROPERTY_CONTAINER(Project)
	DEFINE_ASSET_STATICS("project", "Project", ".te-project", ASSET_CATEGORY_GENERAL);
	DECLARE_FACTORY_ASSET_METADATA()

	static std::filesystem::path getAssetDirectoryFor(std::filesystem::path projectDir);

	DECLARE_ASSET_CONTRUCTORS(Project)
	Project(std::string name, Version version);

	std::filesystem::path getAbsoluteDirectoryPath() const;
	std::filesystem::path getAssetDirectory() const;

	DECLARE_PROPERTY_MUTATORS(std::string, mName, Name)
	DECLARE_PROPERTY_MUTATORS(Version, mVersion, Version)
	DECLARE_PROPERTY_MUTATORS(graphics::PhysicalDevicePreference, mGraphicsDevicePreference, PhysicalDevicePreference)
	DECLARE_PROPERTY_MUTATORS(TypedAssetPath<asset::RenderPass>, mRenderPass, RenderPass)

	// General
	std::string getDisplayName() const;
	graphics::LogicalDeviceInfo getGraphicsDeviceInitInfo() const;

private:
	std::string mName;
	Version mVersion;

	graphics::PhysicalDevicePreference mGraphicsDevicePreference;

	// TODO: Temporary asset referencing to test UI and make initialization easier. Move these to a camera actor/pipeline asset when one is available.
	TypedAssetPath<asset::RenderPass> mRenderPass;

#pragma region Serialization
protected:
	DECLARE_SERIALIZATION_METHOD(write, cereal::JSONOutputArchive, const override);
	DECLARE_SERIALIZATION_METHOD(read, cereal::JSONInputArchive, override);
	DECLARE_SERIALIZATION_METHOD(compile, cereal::PortableBinaryOutputArchive, const override);
	DECLARE_SERIALIZATION_METHOD(decompile, cereal::PortableBinaryInputArchive, override);

	template <typename Archive>
	void serialize(Archive &archive, bool bCheckDefaults) const
	{
		Asset::serialize(archive, bCheckDefaults);
		SAVE_PROPERTY("name", mName);
		SAVE_PROPERTY("version", mVersion);
		SAVE_PROPERTY("gpuPreference", mGraphicsDevicePreference);
		SAVE_PROPERTY("renderPass", mRenderPass);
	}

	template <typename Archive>
	void deserialize(Archive &archive, bool bCheckDefaults)
	{
		Asset::deserialize(archive, bCheckDefaults);
		LOAD_PROPERTY("name", mName);
		LOAD_PROPERTY("version", mVersion);
		LOAD_PROPERTY("gpuPreference", mGraphicsDevicePreference);
		LOAD_PROPERTY("renderPass", mRenderPass);
	}
#pragma endregion

};

typedef std::shared_ptr<Project> ProjectPtrStrong;
typedef std::weak_ptr<Project> ProjectPtrWeak;

NS_END
