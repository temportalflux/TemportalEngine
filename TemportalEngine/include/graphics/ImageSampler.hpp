#pragma once

#include "graphics/DeviceObject.hpp"

#include "math/Vector.hpp"
#include "graphics/types.hpp"

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

	ImageSampler& setFilter(graphics::FilterMode::Enum magnified, graphics::FilterMode::Enum minified);
	ImageSampler& setAddressMode(std::array<graphics::SamplerAddressMode::Enum, 3> uvwMode);
	ImageSampler& setAnistropy(std::optional<f32> anistropy);
	ImageSampler& setBorderColor(graphics::BorderColor::Enum colorSetting);
	ImageSampler& setNormalizeCoordinates(bool enabled);
	ImageSampler& setCompare(std::optional<graphics::CompareOp::Enum> compareOp);
	ImageSampler& setMipLOD(graphics::SamplerLODMode::Enum mode, f32 bias, math::Vector2 range);

	void create() override;
	void* get() override;
	void invalidate() override;
	void resetConfiguration() override;

private:
	graphics::FilterMode::Enum mFilterMag, mFilterMin;
	std::array<graphics::SamplerAddressMode::Enum, 3> mAddressModes;
	std::optional<f32> mAnisotropy;
	graphics::BorderColor::Enum mBorderColor;
	bool mbNormalizeCoordinates;
	std::optional<graphics::CompareOp::Enum> mCompareOp;
	graphics::SamplerLODMode::Enum mMipLODMode;
	f32 mMipLODBias;
	math::Vector2 mMipLODRange;

	vk::UniqueSampler mInternal;

};

NS_END
