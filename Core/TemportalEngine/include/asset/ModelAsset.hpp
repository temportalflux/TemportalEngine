#pragma once

#include "asset/Asset.hpp"

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

	struct Vertex
	{
		math::Vector3Padded position;
		math::Vector2Padded texCoord;

		bool operator==(Vertex const& other) const;

		void save(cereal::PortableBinaryOutputArchive &archive) const;
		void load(cereal::PortableBinaryInputArchive &archive);
	};

	// For text asset
	void setSourcePath(std::filesystem::path sourceFilePath);
	std::filesystem::path getAbsoluteSourcePath() const;
	// For build
	void setSourceBinary(std::vector<Vertex> const& vertices, std::vector<ui32> const& indices);
	// For binary asset
	std::vector<Vertex> const& vertces() const;
	std::vector<ui32> const& indices() const;

	void onPreMoveAsset(std::filesystem::path const& prevAbsolute, std::filesystem::path const& nextAbsolute) override;

private:
	// For text asset
	std::string mSourceFilePath;
	// For binary asset
	std::vector<Vertex> mVertices;
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
