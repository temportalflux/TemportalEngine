#pragma once

#include "asset/Asset.hpp"

#include "graphics/ShaderMetadata.hpp"

FORWARD_DEF(NS_GRAPHICS, class ShaderModule)

NS_ASSET

class Shader : public Asset
{
	friend class cereal::access;

public:
	DEFINE_ASSET_STATICS("shader", "Shader", DEFAULT_ASSET_EXTENSION);
	DECLARE_FACTORY_ASSET_METADATA()

private:
	static std::filesystem::path getSourcePathFrom(std::filesystem::path assetPath);

public:
	Shader() = default;
	Shader(std::filesystem::path filePath);

	ui32 getStage() const;
	void setStage(ui32 value);

	void setBinary(std::vector<ui32> binary, graphics::ShaderMetadata metadata);
	std::optional<graphics::ShaderMetadata> getMetadata();
	void writeSource(std::string content) const;
	std::string readSource() const;

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

	graphics::ShaderMetadata mBinaryMetadata;

#pragma endregion

#pragma region Serialization
protected:
	void write(cereal::JSONOutputArchive &archive) const override;
	void read(cereal::JSONInputArchive &archive) override;
	void compile(cereal::PortableBinaryOutputArchive &archive) const override;
	void decompile(cereal::PortableBinaryInputArchive &archive) override;
#pragma endregion

#pragma region Graphics
public:
	std::shared_ptr<graphics::ShaderModule> makeModule();
#pragma endregion


};

NS_END
