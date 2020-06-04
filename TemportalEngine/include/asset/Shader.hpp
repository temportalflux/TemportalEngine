#pragma once

#include "asset/Asset.hpp"

NS_ASSET

#define AssetType_Shader "shader"

class Shader : public Asset
{
	friend class cereal::access;

public:
	DEFINE_ASSET_TYPE(AssetType_Shader);

	Shader() = default;

private:
	static std::filesystem::path getSourcePathFrom(std::filesystem::path assetPath);

	/**
	 * The Vulkan shader stage flags. Encoded as binary regardless of the file serialization type.
	 * Maps directly to `vk::ShaderStageFlags`.
	 */
	ui32 mStage;

	/**
	 * The compiled source of the shader found at `mSourceFileName`.
	 */
	std::vector<ui32> mSourceBinary;


#pragma region Serialization
public:
	static std::shared_ptr<Asset> createAsset(std::filesystem::path filePath);
	static void onAssetDeleted(std::filesystem::path filePath);
	static std::shared_ptr<Asset> readFromDisk(std::filesystem::path filePath, EAssetSerialization type);
	void writeToDisk(std::filesystem::path filePath, EAssetSerialization type) const override;

private:
	void write(cereal::JSONOutputArchive &archive) const;
	void read(cereal::JSONInputArchive &archive);
	void compile(cereal::PortableBinaryOutputArchive &archive) const;
	void decompile(cereal::PortableBinaryInputArchive &archive);
#pragma endregion

};

NS_END
