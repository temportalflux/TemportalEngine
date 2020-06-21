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
	this->mInternal.reset();
}

ImageSampler& ImageSampler::setFilter(vk::Filter magnified, vk::Filter minified)
{
	this->mFilterMag = magnified;
	this->mFilterMin = minified;
	return *this;
}

ImageSampler& ImageSampler::setAddressMode(math::Vector<ui8, 3> uvwMode)
{
	this->mAddressModes = uvwMode;
	return *this;
}

ImageSampler& ImageSampler::setAnistropy(std::optional<f32> anistropy)
{
	this->mAnistropy = anistropy;
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
		.setAddressModeU((vk::SamplerAddressMode)this->mAddressModes.x())
		.setAddressModeV((vk::SamplerAddressMode)this->mAddressModes.y())
		.setAddressModeW((vk::SamplerAddressMode)this->mAddressModes.z())
		.setAnisotropyEnable((bool)this->mAnistropy)
		.setMaxAnisotropy(this->mAnistropy ? *this->mAnistropy : 1.0f)
		.setBorderColor(this->mBorderColor)
		.setUnnormalizedCoordinates(this->mbNormalizeCoordinates)
		.setCompareEnable((bool)this->mCompareOp)
		.setCompareOp(this->mCompareOp ? *this->mCompareOp : vk::CompareOp::eAlways)
		.setMipmapMode(this->mMipLODMode)
		.setMipLodBias(this->mMipLODBias)
		.setMinLod(this->mMipLODRange.x())
		.setMaxLod(this->mMipLODRange.y())
	);
	return *this;
}

void ImageSampler::invalidate()
{
}
