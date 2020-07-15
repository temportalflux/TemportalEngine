#pragma once

#include "asset/Asset.hpp"

#include "graphics/types.hpp"
#include "math/Vector.hpp"
#include "cereal/optional.hpp"
#include "cereal/list.hpp"
#include "cereal/mathVector.hpp"

NS_ASSET

class TextureSampler : public Asset
{
	friend class cereal::access;

public:
	DEFINE_ASSET_STATICS("sampler", "Texture Sampler", DEFAULT_ASSET_EXTENSION);
	DECLARE_FACTORY_ASSET_METADATA()

	TextureSampler() = default;
	CREATE_NEWASSET_CONSTRUCTOR(TextureSampler)
		, mFilterModeMagnified(graphics::FilterMode::Enum::Nearest)
		, mFilterModeMinified(graphics::FilterMode::Enum::Nearest)
		, mAddressModesUVW({})
		, mBorderColor(graphics::BorderColor::Enum::BlackOpaqueInt)
		, mAnisotropy(std::nullopt)
		, mbNormalizeCoordinates(false)
		, mCompareOp(std::nullopt)
		, mMipLODMode(graphics::SamplerLODMode::Enum::Nearest)
		, mMipLODBias(0)
		, mMipLODRange({ 0, 0 })
	{}

	void setFilterModeMagnified(graphics::FilterMode::Enum v);
	graphics::FilterMode::Enum getFilterModeMagnified() const;
	void setFilterModeMinified(graphics::FilterMode::Enum v);
	graphics::FilterMode::Enum getFilterModeMinified() const;
	void setAddressModes(std::array<graphics::SamplerAddressMode::Enum, 3> modes);
	std::array<graphics::SamplerAddressMode::Enum, 3> getAddressModes() const;
	void setAnisotropy(std::optional<f32> v);
	std::optional<f32> getAnisotropy() const;
	void setBorderColor(graphics::BorderColor::Enum v);
	graphics::BorderColor::Enum getBorderColor() const;
	void setCoordinatesNormalized(bool enabled);
	bool areCoordinatesNormalized() const;
	void setCompareOperation(std::optional<graphics::CompareOp::Enum> v);
	std::optional<graphics::CompareOp::Enum> getCompareOperation() const;
	void setLodMode(graphics::SamplerLODMode::Enum v);
	graphics::SamplerLODMode::Enum getLodMode() const;
	void setLodBias(f32 v);
	f32 getLodBias() const;
	void setLodRange(math::Vector2 v);
	math::Vector2 getLodRange() const;

private:
	graphics::FilterMode::Enum mFilterModeMagnified, mFilterModeMinified;
	std::array<graphics::SamplerAddressMode::Enum, 3> mAddressModesUVW;
	std::optional<f32> mAnisotropy;
	graphics::BorderColor::Enum mBorderColor;
	bool mbNormalizeCoordinates;
	std::optional<graphics::CompareOp::Enum> mCompareOp;
	graphics::SamplerLODMode::Enum mMipLODMode;
	f32 mMipLODBias;
	math::Vector2 mMipLODRange;

#pragma region Serialization
protected:
	DECLARE_SERIALIZATION_METHOD(write, cereal::JSONOutputArchive, const override);
	DECLARE_SERIALIZATION_METHOD(read, cereal::JSONInputArchive, override);
	DECLARE_SERIALIZATION_METHOD(compile, cereal::PortableBinaryOutputArchive, const override);
	DECLARE_SERIALIZATION_METHOD(decompile, cereal::PortableBinaryInputArchive, override);

	template <typename Archive>
	void serialize(Archive &archive) const
	{
		Asset::serialize(archive);
		archive(cereal::make_nvp("filterMagnified", this->mFilterModeMagnified));
		archive(cereal::make_nvp("filterMinified", this->mFilterModeMinified));
		archive(cereal::make_nvp("addressModes", this->mAddressModesUVW));
		archive(cereal::make_nvp("anisotropy", this->mAnisotropy));
		archive(cereal::make_nvp("borderColor", this->mBorderColor));
		archive(cereal::make_nvp("normalizeCoords", this->mbNormalizeCoordinates));
		archive(cereal::make_nvp("compareOperation", this->mCompareOp));
		archive(cereal::make_nvp("lodMode", this->mMipLODMode));
		archive(cereal::make_nvp("lodBias", this->mMipLODBias));
		archive(cereal::make_nvp("lodRange", this->mMipLODRange));
	}

	template <typename Archive>
	void deserialize(Archive &archive)
	{
		Asset::deserialize(archive);
		archive(cereal::make_nvp("filterMagnified", this->mFilterModeMagnified));
		archive(cereal::make_nvp("filterMinified", this->mFilterModeMinified));
		archive(cereal::make_nvp("addressModes", this->mAddressModesUVW));
		archive(cereal::make_nvp("anisotropy", this->mAnisotropy));
		archive(cereal::make_nvp("borderColor", this->mBorderColor));
		archive(cereal::make_nvp("normalizeCoords", this->mbNormalizeCoordinates));
		archive(cereal::make_nvp("compareOperation", this->mCompareOp));
		archive(cereal::make_nvp("lodMode", this->mMipLODMode));
		archive(cereal::make_nvp("lodBias", this->mMipLODBias));
		archive(cereal::make_nvp("lodRange", this->mMipLODRange));
	}
#pragma endregion

};

NS_END
