#include "asset/TextureSampler.hpp"

#include "asset/AssetManager.hpp"

#include <cereal/archives/json.hpp>
#include <cereal/archives/portable_binary.hpp>

using namespace asset;

DEFINE_FACTORY_ASSET_METADATA(TextureSampler)

void TextureSampler::setFilterModeMagnified(graphics::FilterMode::Enum v) { this->mFilterModeMagnified = v; }
void TextureSampler::setFilterModeMinified(graphics::FilterMode::Enum v) { this->mFilterModeMinified = v; }
void TextureSampler::setAddressModes(std::array<graphics::SamplerAddressMode::Enum, 3> modes) { this->mAddressModesUVW = modes; }
void TextureSampler::setAnisotropy(std::optional<f32> v) { this->mAnisotropy = v; }
void TextureSampler::setBorderColor(graphics::BorderColor::Enum v) { this->mBorderColor = v; }
void TextureSampler::setCoordinatesNormalized(bool enabled) { this->mbNormalizeCoordinates = enabled; }
void TextureSampler::setCompareOperation(std::optional<graphics::CompareOp::Enum> v) { this->mCompareOp = v; }
void TextureSampler::setLodMode(graphics::SamplerLODMode::Enum v) { this->mMipLODMode = v; }
void TextureSampler::setLodBias(f32 v) { this->mMipLODBias = v; }
void TextureSampler::setLodRange(math::Vector2 v) { this->mMipLODRange = v; }

graphics::FilterMode::Enum TextureSampler::getFilterModeMagnified() const
{
	return this->mFilterModeMagnified;
}

graphics::FilterMode::Enum TextureSampler::getFilterModeMinified() const
{
	return this->mFilterModeMinified;
}

std::array<graphics::SamplerAddressMode::Enum, 3> TextureSampler::getAddressModes() const
{
	return this->mAddressModesUVW;
}

std::optional<f32> TextureSampler::getAnisotropy() const
{
	return this->mAnisotropy;
}

graphics::BorderColor::Enum TextureSampler::getBorderColor() const
{
	return this->mBorderColor;
}

bool TextureSampler::areCoordinatesNormalized() const
{
	return this->mbNormalizeCoordinates;
}

std::optional<graphics::CompareOp::Enum> TextureSampler::getCompareOperation() const
{
	return this->mCompareOp;
}

graphics::SamplerLODMode::Enum TextureSampler::getLodMode() const
{
	return this->mMipLODMode;
}

f32 TextureSampler::getLodBias() const
{
	return this->mMipLODBias;
}

math::Vector2 TextureSampler::getLodRange() const
{
	return this->mMipLODRange;
}

CREATE_DEFAULT_SERIALIZATION_DEFINITION(const, TextureSampler::write, cereal::JSONOutputArchive, TextureSampler::serialize);
CREATE_DEFAULT_SERIALIZATION_DEFINITION(, TextureSampler::read, cereal::JSONInputArchive, TextureSampler::deserialize);
CREATE_DEFAULT_SERIALIZATION_DEFINITION(const, TextureSampler::compile, cereal::PortableBinaryOutputArchive, TextureSampler::serialize);
CREATE_DEFAULT_SERIALIZATION_DEFINITION(, TextureSampler::decompile, cereal::PortableBinaryInputArchive, TextureSampler::deserialize);
