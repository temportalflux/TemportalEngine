#pragma once

#include "graphics/DeviceObject.hpp"

#include <vulkan/vulkan.hpp>

NS_GRAPHICS

class DescriptorPool : public DeviceObject
{
	friend class GraphicsDevice;

public:
	DescriptorPool();

	DescriptorPool& setFlags(vk::DescriptorPoolCreateFlags flags);
	DescriptorPool& setAllocationMultiplier(ui32 const setCount);

	// Map of `vk::DescriptorType` to the number of available allocations
	// (will later be multiplied by the number of frames)
	DescriptorPool& setPoolSize(ui32 maxSets, std::unordered_map<vk::DescriptorType, ui32> const &sizes);

	void create() override;
	void* get() override;
	void invalidate() override;
	void resetConfiguration() override;

private:
	vk::DescriptorPoolCreateFlags mFlags;
	ui32 mMaxSets;
	ui32 mAllocationMultiplier;
	std::unordered_map<vk::DescriptorType, ui32> mAvailableAllocationsPerType;

	vk::UniqueDescriptorPool mInternal;

};

NS_END
