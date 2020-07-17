#pragma once

#include "graphics/DeviceObject.hpp"

#include "math/Vector.hpp"

#include <vulkan/vulkan.hpp>

NS_GRAPHICS
class GraphicsDevice;

class ImageSampler : public DeviceObject
{

public:
	ImageSampler();
	~ImageSampler();
	ImageSampler(ImageSampler &&other);
	ImageSampler& operator=(ImageSampler &&other);

	ImageSampler& setFilter(vk::Filter magnified, vk::Filter minified);
	ImageSampler& setAddressMode(std::array<vk::SamplerAddressMode, 3> uvwMode);
	ImageSampler& setAnistropy(std::optional<f32> anistropy);
	ImageSampler& setBorderColor(vk::BorderColor colorSetting);
	ImageSampler& setNormalizeCoordinates(bool enabled);
	ImageSampler& setCompare(std::optional<vk::CompareOp> compareOp);
	ImageSampler& setMipLOD(vk::SamplerMipmapMode mode, f32 bias, math::Vector2 range);

	void create() override;
	void* get() override;
	void invalidate() override;
	void resetConfiguration() override;

private:
	vk::Filter mFilterMag, mFilterMin;
	std::array<vk::SamplerAddressMode, 3> mAddressModes;
	std::optional<f32> mAnisotropy;
	vk::BorderColor mBorderColor;
	bool mbNormalizeCoordinates;
	std::optional<vk::CompareOp> mCompareOp;
	vk::SamplerMipmapMode mMipLODMode;
	f32 mMipLODBias;
	math::Vector2 mMipLODRange;

	vk::UniqueSampler mInternal;

};

NS_END
