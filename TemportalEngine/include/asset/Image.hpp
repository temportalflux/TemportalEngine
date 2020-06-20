#pragma once

#include "asset/Asset.hpp"

#include "math/Vector.hpp"

NS_ASSET

#define AssetType_Image "image"

class Image : public Asset
{
	friend class cereal::access;

public:
	DEFINE_ASSET_TYPE(AssetType_Image);
	DECLARE_NEWASSET_FACTORY()
	DECLARE_EMPTYASSET_FACTORY()

public:
	Image() = default;
	Image(std::filesystem::path filePath);

	void setSourcePath(std::filesystem::path sourceFilePath);
	std::filesystem::path getAbsoluteSourcePath() const;
	void setSourceBinary(std::vector<ui8> const &binary, math::Vector2UInt size);

private:
	std::string mSourceFilePath;
	// Binary data of the source (accessible when asset is compiled)
	// Always 4 (RGBA) ui8 values per pixel (total size of buffer equals width*height*4)
	std::vector<ui8> mSourceBinary;
	// dimensions of the image
	math::Vector2UInt mSourceSize;

#pragma region Serialization
protected:
	void write(cereal::JSONOutputArchive &archive) const override;
	void read(cereal::JSONInputArchive &archive) override;
	void compile(cereal::PortableBinaryOutputArchive &archive) const override;
	void decompile(cereal::PortableBinaryInputArchive &archive) override;
#pragma endregion

};

NS_END
