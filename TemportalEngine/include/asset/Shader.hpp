#pragma once

#include "asset/Asset.hpp"

#include <optional>

NS_ASSET

#define AssetType_Shader "shader"

class Shader : public Asset
{
	friend class cereal::access;

public:
	DEFINE_ASSET_TYPE(AssetType_Shader);

	static asset::AssetPtrStrong createNewAsset(std::filesystem::path filePath);
	static asset::AssetPtrStrong createEmptyAsset();
	static void onAssetDeleted(std::filesystem::path filePath);

private:
	static std::filesystem::path getSourcePathFrom(std::filesystem::path assetPath);

public:
	Shader() = default;
	Shader(std::filesystem::path filePath);

	ui32 getStage() const { return this->mStage; }

	std::string readSource() const;
	void writeSource(std::string content) const;

#pragma region Properties
private:

	/**
	 * The Vulkan shader stage flags. Encoded as binary regardless of the file serialization type.
	 * Maps directly to `vk::ShaderStageFlags`.
	 */
	ui32 mStage;

	/**
	 * The compiled source of the shader found at `mSourceFileName`.
	 */
	std::vector<ui32> mSourceBinary;
#pragma endregion

#pragma region Serialization
protected:
	void write(cereal::JSONOutputArchive &archive) const override;
	void read(cereal::JSONInputArchive &archive) override;
	void compile(cereal::PortableBinaryOutputArchive &archive) const override;
	void decompile(cereal::PortableBinaryInputArchive &archive) override;
#pragma endregion

};

NS_END
