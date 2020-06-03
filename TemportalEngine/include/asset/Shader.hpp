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
	/**
	 * The Vulkan shader stage flags. Encoded as binary regardless of the file serialization type.
	 * Maps directly to `vk::ShaderStageFlags`.
	 */
	ui32 mStage;

	/**
	 * This is the file name of both this asset and the underlying source file that comprises the actual shader.
	 * The extension is appended depending on the file (an asset extension for this object, `.glsl` for the source).
	 * When this asset object is compiled/written as binary, the source file `<mFileName>.glsl`
	 * is compiled and embedded to the `mSourceBinary` property.
	 * The content of the source file is also used to parse things like vertex attribute descriptions.
	 */
	// TODO: This is derived from `mFilePath` and is outdated. Make a getter function.
	std::string mFileName;

	/**
	 * The compiled source of the shader found at `mSourceFileName`.
	 */
	std::vector<ui32> mSourceBinary;

#pragma region Serialization
public:
	static std::shared_ptr<Asset> createAsset(std::filesystem::path filePath);
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
