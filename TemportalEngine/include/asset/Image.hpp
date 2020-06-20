#pragma once

#include "asset/Asset.hpp"

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
	void setSourceBinary(std::vector<ui8> const &binary);

private:
	std::string mSourceFilePath;
	std::vector<ui8> mSourceBinary;

#pragma region Serialization
protected:
	void write(cereal::JSONOutputArchive &archive) const override;
	void read(cereal::JSONInputArchive &archive) override;
	void compile(cereal::PortableBinaryOutputArchive &archive) const override;
	void decompile(cereal::PortableBinaryInputArchive &archive) override;
#pragma endregion

};

NS_END
