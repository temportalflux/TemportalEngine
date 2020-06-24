#pragma once

#include "asset/Asset.hpp"

NS_ASSET

#define AssetType_EditorSettings "editorSettings"

class Settings : public Asset
{
	friend class cereal::access;

public:
	DEFINE_ASSET_TYPE(AssetType_EditorSettings);
	DECLARE_NEWASSET_FACTORY()
	DECLARE_EMPTYASSET_FACTORY()
	
	Settings() = default;
	CREATE_NEWASSET_CONSTRUCTOR(Settings) {}

	// Returns the relative path from `asset::Project::getAbsoluteDirectoryPath`
	std::string getOutputDirectory() const;
	void setOutputDirectory(std::string path);

private:
	// A relative path from `asset::Project::getAbsoluteDirectoryPath` to which asset files should be built
	std::string mOutputDirectoryPath;

#pragma region Serialization
protected:
	DECLARE_SERIALIZATION_METHOD(write, cereal::JSONOutputArchive, const override);
	DECLARE_SERIALIZATION_METHOD(read, cereal::JSONInputArchive, override);
	NOOP_SERIALIZATION_METHOD(compile, cereal::PortableBinaryOutputArchive, const override);
	NOOP_SERIALIZATION_METHOD(decompile, cereal::PortableBinaryInputArchive, override);
#pragma endregion
};

NS_END
