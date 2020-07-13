#pragma once

#include "asset/Asset.hpp"

#include "math/Vector.hpp"

NS_ASSET

class Texture : public Asset
{
	friend class cereal::access;

public:
	DEFINE_ASSET_STATICS("texture", "Texture", DEFAULT_ASSET_EXTENSION);
	DECLARE_FACTORY_ASSET_METADATA()

public:
	Texture() = default;
	Texture(std::filesystem::path filePath);

	void setSourcePath(std::filesystem::path sourceFilePath);
	std::filesystem::path getAbsoluteSourcePath() const;
	void setSourceBinary(std::vector<ui8> const &binary, math::Vector2UInt size);
	std::vector<ui8> getSourceBinary() const;
	math::Vector2UInt getSourceSize() const;
	uSize getSourceMemorySize() const;

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
