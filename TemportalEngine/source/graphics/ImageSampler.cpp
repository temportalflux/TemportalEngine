#include "graphics/ImageSampler.hpp"

#include "graphics/LogicalDevice.hpp"

using namespace graphics;

ImageSampler::ImageSampler()
	: mbNormalizeCoordinates(true)
{
	this->setMipLOD(
		vk::SamplerMipmapMode::eLinear,
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

ImageSampler& ImageSampler::setFilter(vk::Filter magnified, vk::Filter minified)
{
	this->mFilterMag = magnified;
	this->mFilterMin = minified;
	return *this;
}

ImageSampler& ImageSampler::setAddressMode(std::array<vk::SamplerAddressMode, 3> uvwMode)
{
	this->mAddressModes = uvwMode;
	return *this;
}

ImageSampler& ImageSampler::setAnistropy(std::optional<f32> anistropy)
{
	this->mAnisotropy = anistropy;
	return *this;
}

ImageSampler& ImageSampler::setBorderColor(vk::BorderColor colorSetting)
{
	this->mBorderColor = colorSetting;
	return *this;
}

ImageSampler& ImageSampler::setNormalizeCoordinates(bool enabled)
{
	this->mbNormalizeCoordinates = enabled;
	return *this;
}

ImageSampler& ImageSampler::setCompare(std::optional<vk::CompareOp> compareOp)
{
	this->mCompareOp = compareOp;
	return *this;
}

ImageSampler& ImageSampler::setMipLOD(vk::SamplerMipmapMode mode, f32 bias, math::Vector2 range)
{
	this->mMipLODMode = mode;
	this->mMipLODBias = bias;
	this->mMipLODRange = range;
	return *this;
}

ImageSampler& ImageSampler::create(LogicalDevice *device)
{
	this->mInternal = device->mDevice->createSamplerUnique(
		vk::SamplerCreateInfo()
		.setMagFilter(this->mFilterMag).setMinFilter(this->mFilterMin)
		.setAddressModeU(this->mAddressModes[0])
		.setAddressModeV(this->mAddressModes[1])
		.setAddressModeW(this->mAddressModes[2])
		.setAnisotropyEnable((bool)this->mAnisotropy)
		.setMaxAnisotropy(this->mAnisotropy ? *this->mAnisotropy : 1.0f)
		.setBorderColor(this->mBorderColor)
		.setUnnormalizedCoordinates(!this->mbNormalizeCoordinates)
		.setCompareEnable((bool)this->mCompareOp)
		.setCompareOp(this->mCompareOp ? *this->mCompareOp : vk::CompareOp::eAlways)
		.setMipmapMode(this->mMipLODMode)
		.setMipLodBias(this->mMipLODBias)
		.setMinLod(this->mMipLODRange.x())
		.setMaxLod(this->mMipLODRange.y())
	);
	return *this;
}

void* ImageSampler::get()
{
	return &this->mInternal.get();
}

void ImageSampler::invalidate()
{
	this->mInternal.reset();
}
