#pragma once

#include "TemportalEnginePCH.hpp"

#include <vulkan/vulkan.hpp>

NS_GRAPHICS
class GraphicsDevice;

class DescriptorPool
{
	friend class GraphicsDevice;

public:
	DescriptorPool() = default;

	DescriptorPool& setFlags(vk::DescriptorPoolCreateFlags flags);
	// Map of `vk::DescriptorType` to the number of available allocations
	// (will later be multiplied by the number of frames)
	DescriptorPool& setPoolSize(ui32 maxSets, std::unordered_map<vk::DescriptorType, ui32> const &sizes);

	DescriptorPool& create(std::shared_ptr<GraphicsDevice> device, ui32 const &frameCount);
	void* get();
	void invalidate();

private:
	vk::DescriptorPoolCreateFlags mFlags;
	ui32 mMaxSets;
	std::unordered_map<vk::DescriptorType, ui32> mAvailableAllocationsPerType;
	vk::UniqueDescriptorPool mInternal;

};

NS_END
