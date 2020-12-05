#pragma once

#include "asset/Asset.hpp"

#include "render/ModelVertex.hpp"

NS_ASSET

class Model : public Asset
{
	friend class cereal::access;

public:
	DEFINE_ASSET_STATICS("model", "Model", DEFAULT_ASSET_EXTENSION, ASSET_CATEGORY_GRAPHICS);
	DECLARE_FACTORY_ASSET_METADATA()

public:
	Model() = default;
	Model(std::filesystem::path filePath);

	// For text asset
	void setSourcePath(std::filesystem::path sourceFilePath);
	std::filesystem::path getAbsoluteSourcePath() const;
	// For build
	void setSourceBinary(std::vector<ModelVertex> const& vertices, std::vector<ui32> const& indices);
	// For binary asset
	std::vector<ModelVertex> const& vertices() const;
	std::vector<ui32> const& indices() const;

	void onPreMoveAsset(std::filesystem::path const& prevAbsolute, std::filesystem::path const& nextAbsolute) override;

private:
	// For text asset
	std::string mSourceFilePath;
	// For binary asset
	std::vector<ModelVertex> mVertices;
	std::vector<ui32> mIndices;

#pragma region Serialization
protected:
	void write(cereal::JSONOutputArchive &archive, bool bCheckDefaults) const override;
	void read(cereal::JSONInputArchive &archive, bool bCheckDefaults) override;
	void compile(cereal::PortableBinaryOutputArchive &archive, bool bCheckDefaults) const override;
	void decompile(cereal::PortableBinaryInputArchive &archive, bool bCheckDefaults) override;
#pragma endregion

};

NS_END
