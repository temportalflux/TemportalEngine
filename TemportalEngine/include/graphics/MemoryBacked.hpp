#pragma once

#include "TemportalEnginePCH.hpp"

#include <vulkan/vulkan.hpp>

NS_GRAPHICS
class GraphicsDevice;

class MemoryBacked
{
	friend class GraphicsDevice;

public:
	MemoryBacked() = default;

	MemoryBacked& setMemoryRequirements(vk::MemoryPropertyFlags flags);

	uSize getMemorySize() const;

	void createMemory(std::shared_ptr<GraphicsDevice> device, vk::MemoryRequirements const &req);
	void invalidate();

	void write(std::shared_ptr<GraphicsDevice> device, ui64 offset, void* src, ui64 size, bool bClear=false);

protected:
	vk::MemoryPropertyFlags mMemoryFlags;
	vk::UniqueDeviceMemory mBufferMemory;
	uSize mMemorySize;

	std::optional<ui32> findMemoryType(std::shared_ptr<GraphicsDevice> device, ui32 typeFilter, vk::MemoryPropertyFlags propertyFlags);
	virtual void bind(std::shared_ptr<GraphicsDevice> device, ui64 offset = 0) = 0;

};

NS_END
