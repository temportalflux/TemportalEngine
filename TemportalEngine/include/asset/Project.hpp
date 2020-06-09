#pragma once

#include "asset/Asset.hpp"

#include "version.h"
#include "graphics/PhysicalDevicePreference.hpp"
#include "graphics/LogicalDeviceInfo.hpp"

NS_ASSET
class Shader;

#define AssetType_Project "project"

class Project : public Asset
{
	friend class cereal::access;
	
public:
	DEFINE_ASSET_TYPE(AssetType_Project);
	DECLARE_NEWASSET_FACTORY()
	DECLARE_EMPTYASSET_FACTORY()

	static std::filesystem::path getAssetDirectoryFor(std::filesystem::path projectDir);

	Project() = default;
	Project(std::filesystem::path path);
	Project(std::string name, Version version);

	std::filesystem::path getAbsoluteDirectoryPath() const;
	std::filesystem::path getAssetDirectory() const;
	
	// General
	std::string getName() const;
	void setName(std::string value);
	Version getVersion() const;
	void setVersion(Version value);
	std::string getDisplayName() const;
	
	// Graphics
	graphics::PhysicalDevicePreference getPhysicalDevicePreference() const;
	void setPhysicalDevicePreference(graphics::PhysicalDevicePreference const &prefs);

	graphics::LogicalDeviceInfo getGraphicsDeviceInitInfo() const;

private:
	std::string mName;
	Version mVersion;

	graphics::PhysicalDevicePreference mGraphicsDevicePreference;

	// TODO: Temporary asset referencing to test UI and make initialization easier. Move these to a camera actor when one is available.
	//TypedAssetPath<asset::Shader> mVertexShader;

#pragma region Serialization
protected:
	DECLARE_SERIALIZATION_METHOD(write, cereal::JSONOutputArchive, const override);
	DECLARE_SERIALIZATION_METHOD(read, cereal::JSONInputArchive, override);
	DECLARE_SERIALIZATION_METHOD(compile, cereal::PortableBinaryOutputArchive, const override);
	DECLARE_SERIALIZATION_METHOD(decompile, cereal::PortableBinaryInputArchive, override);

	template <typename Archive>
	void serialize(Archive &archive) const
	{
		Asset::serialize(archive);
		// General
		archive(
			cereal::make_nvp("name", this->mName),
			cereal::make_nvp("version", this->mVersion)
		);
		// Graphics
		archive(
			cereal::make_nvp("gpuPreference", this->mGraphicsDevicePreference)
		);
	}

	template <typename Archive>
	void deserialize(Archive &archive)
	{
		Asset::deserialize(archive);
		// General
		archive(
			cereal::make_nvp("name", this->mName),
			cereal::make_nvp("version", this->mVersion)
		);
		// Graphics
		archive(
			cereal::make_nvp("gpuPreference", this->mGraphicsDevicePreference)
		);
	}
#pragma endregion

};

typedef std::shared_ptr<Project> ProjectPtrStrong;
typedef std::weak_ptr<Project> ProjectPtrWeak;

NS_END
