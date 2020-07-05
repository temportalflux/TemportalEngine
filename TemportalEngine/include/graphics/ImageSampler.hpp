#pragma once

#include "TemportalEnginePCH.hpp"

#include "math/Vector.hpp"

#include <vulkan/vulkan.hpp>

NS_GRAPHICS
class GraphicsDevice;

class ImageSampler
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

	ImageSampler& create(std::shared_ptr<GraphicsDevice> device);
	void* get();
	void invalidate();

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
