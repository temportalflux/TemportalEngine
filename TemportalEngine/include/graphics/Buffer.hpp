#pragma once

#include "graphics/MemoryBacked.hpp"

#include <vulkan/vulkan.hpp>

NS_GRAPHICS
class GraphicsDevice;

class Buffer : public MemoryBacked
{
	friend class GraphicsDevice;

public:
	Buffer() = default;
	Buffer(Buffer &&other);
	Buffer& operator=(Buffer &&other);

	Buffer& setUsage(vk::BufferUsageFlags flags);
	Buffer& setSize(uSize size);
	uSize getSize() const;

	void create(std::shared_ptr<GraphicsDevice> device);
	void destroy();

	void* get();

private:
	vk::BufferUsageFlags mUsageFlags;
	uSize mSize;
	vk::UniqueBuffer mInternal;

	void bind(std::shared_ptr<GraphicsDevice> device, ui64 offset) override;

};

NS_END
