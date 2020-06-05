#pragma once

#include "asset/Asset.hpp"

NS_ASSET

#define AssetType_EditorSettings "editorSettings"

class Settings : public Asset
{
	friend class cereal::access;

public:
	DEFINE_ASSET_TYPE(AssetType_EditorSettings);

	static asset::AssetPtrStrong createNewAsset(std::filesystem::path filePath);
	static asset::AssetPtrStrong createEmptyAsset();
	
	Settings() = default;
	Settings(std::filesystem::path filePath);

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
	void compile(cereal::PortableBinaryOutputArchive &archive) const override {}
	void decompile(cereal::PortableBinaryInputArchive &archive) override {}
#pragma endregion
};

NS_END
