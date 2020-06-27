#include "asset/TextureSampler.hpp"

#include "asset/AssetManager.hpp"

#include <cereal/archives/json.hpp>
#include <cereal/archives/portable_binary.hpp>

using namespace asset;

DEFINE_FACTORY_ASSET_METADATA(TextureSampler)

graphics::FilterMode::Enum TextureSampler::getFilterModeMagnified() const
{
	return this->mFilterModeMagnified;
}

graphics::FilterMode::Enum TextureSampler::getFilterModeMinified() const
{
	return this->mFilterModeMinified;
}

std::array<graphics::SamplerAddressMode::Enum, 3> TextureSampler::getAddressMode() const
{
	return this->mAddressModesUVW;
}

std::optional<f32> TextureSampler::getAnistropy() const
{
	return this->mAnistropy;
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
