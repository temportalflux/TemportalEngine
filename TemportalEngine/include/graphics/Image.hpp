#pragma once

#include "graphics/DeviceObject.hpp"
#include "graphics/MemoryAllocated.hpp"

#include "math/Vector.hpp"

#include <vulkan/vulkan.hpp>

NS_GRAPHICS
class GraphicsDevice;

class Image : public DeviceObject, public MemoryAllocated
{
	friend class GraphicsDevice;

public:
	Image() : mType(vk::ImageType::e2D) {}

	vk::ImageType getType() const { return this->mType; }
	Image& setFormat(vk::Format format);
	vk::Format getFormat() const;
	bool hasStencilComponent() const;
	Image& setTiling(vk::ImageTiling tiling);
	Image& setUsage(vk::ImageUsageFlags usage);
	Image& setSize(math::Vector3UInt const &size);
	math::Vector3UInt getSize() const;

	uSize getExpectedDataCount() const;

	void create() override;
	void* get() override;
	void invalidate() override;
	void resetConfiguration() override;

	void bindMemory() override;
	void transitionLayout(vk::ImageLayout prev, vk::ImageLayout next, class CommandPool* transientPool);
	void writeImage(void* data, uSize size, class CommandPool* transientPool);

private:
	vk::ImageType mType;
	vk::Format mFormat;
	vk::ImageTiling mTiling;
	vk::ImageUsageFlags mUsage;
	math::Vector3UInt mImageSize;

	vk::UniqueImage mInternal;

	vk::MemoryRequirements getRequirements() const override;

	void copyBufferToImage(class Buffer *src, class CommandPool* transientPool);

};

NS_END
