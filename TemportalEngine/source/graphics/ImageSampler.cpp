#include "graphics/ImageSampler.hpp"

#include "graphics/GraphicsDevice.hpp"
#include "graphics/VulkanApi.hpp"

using namespace graphics;

ImageSampler::ImageSampler()
	: DeviceObject()
	, mbNormalizeCoordinates(true)
{
	this->setMipLOD(
		graphics::SamplerLODMode::Enum::Linear,
		0.0f, math::Vector2::ZERO
	);
}

ImageSampler::~ImageSampler()
{
	this->invalidate();
}

ImageSampler::ImageSampler(ImageSampler &&other)
{
	*this = std::move(other);
}

ImageSampler& ImageSampler::operator=(ImageSampler &&other)
{
	this->setDevice(other.device());

	this->mFilterMag = other.mFilterMag;
	this->mFilterMin = other.mFilterMin;
	this->mAddressModes = other.mAddressModes;
	this->mAnisotropy = other.mAnisotropy;
	this->mBorderColor = other.mBorderColor;
	this->mbNormalizeCoordinates = other.mbNormalizeCoordinates;
	this->mCompareOp = other.mCompareOp;
	this->mMipLODMode = other.mMipLODMode;
	this->mMipLODBias = other.mMipLODBias;
	this->mMipLODRange = other.mMipLODRange;
	this->mInternal = std::move(other.mInternal);
	return *this;
}

ImageSampler& ImageSampler::setFilter(graphics::FilterMode::Enum magnified, graphics::FilterMode::Enum minified)
{
	this->mFilterMag = magnified;
	this->mFilterMin = minified;
	return *this;
}

ImageSampler& ImageSampler::setAddressMode(std::array<graphics::SamplerAddressMode::Enum, 3> uvwMode)
{
	this->mAddressModes = uvwMode;
	return *this;
}

ImageSampler& ImageSampler::setAnistropy(std::optional<f32> anistropy)
{
	this->mAnisotropy = anistropy;
	return *this;
}

ImageSampler& ImageSampler::setBorderColor(graphics::BorderColor::Enum colorSetting)
{
	this->mBorderColor = colorSetting;
	return *this;
}

ImageSampler& ImageSampler::setNormalizeCoordinates(bool enabled)
{
	this->mbNormalizeCoordinates = enabled;
	return *this;
}

ImageSampler& ImageSampler::setCompare(std::optional<graphics::CompareOp::Enum> compareOp)
{
	this->mCompareOp = compareOp;
	return *this;
}

ImageSampler& ImageSampler::setMipLOD(graphics::SamplerLODMode::Enum mode, f32 bias, math::Vector2 range)
{
	this->mMipLODMode = mode;
	this->mMipLODBias = bias;
	this->mMipLODRange = range;
	return *this;
}

void ImageSampler::create()
{
	this->mInternal = this->device()->createSampler(
		vk::SamplerCreateInfo()
		.setMagFilter((vk::Filter)this->mFilterMag).setMinFilter((vk::Filter)this->mFilterMin)
		.setAddressModeU((vk::SamplerAddressMode)this->mAddressModes[0])
		.setAddressModeV((vk::SamplerAddressMode)this->mAddressModes[1])
		.setAddressModeW((vk::SamplerAddressMode)this->mAddressModes[2])
		.setAnisotropyEnable((bool)this->mAnisotropy)
		.setMaxAnisotropy(this->mAnisotropy ? *this->mAnisotropy : 1.0f)
		.setBorderColor((vk::BorderColor)this->mBorderColor)
		.setUnnormalizedCoordinates(!this->mbNormalizeCoordinates)
		.setCompareEnable((bool)this->mCompareOp)
		.setCompareOp(this->mCompareOp ? (vk::CompareOp)*this->mCompareOp : vk::CompareOp::eAlways)
		.setMipmapMode((vk::SamplerMipmapMode)this->mMipLODMode)
		.setMipLodBias(this->mMipLODBias)
		.setMinLod(this->mMipLODRange.x())
		.setMaxLod(this->mMipLODRange.y())
	);
}

void* ImageSampler::get()
{
	return &this->mInternal.get();
}

void ImageSampler::invalidate()
{
	this->mInternal.reset();
}

void ImageSampler::resetConfiguration()
{
	this->mFilterMag = graphics::FilterMode::Enum::Nearest;
	this->mFilterMin = graphics::FilterMode::Enum::Nearest;
	this->mAddressModes = {
		graphics::SamplerAddressMode::Enum::Repeat,
		graphics::SamplerAddressMode::Enum::Repeat,
		graphics::SamplerAddressMode::Enum::Repeat
	};
	this->mAnisotropy = std::nullopt;
	this->mBorderColor = graphics::BorderColor::Enum::BlackOpaqueInt;
	this->mbNormalizeCoordinates = false;
	this->mCompareOp = std::nullopt;
	this->mMipLODMode = graphics::SamplerLODMode::Enum::Nearest;
	this->mMipLODBias = 0;
	this->mMipLODRange = { 0, 0 };
}
