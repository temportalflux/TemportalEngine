#pragma once

#include "TemportalEnginePCH.hpp"

#include <vulkan/vulkan.hpp>

NS_GRAPHICS
class LogicalDevice;

class DescriptorPool
{

public:
	DescriptorPool() = default;

	// Map of `vk::DescriptorType` to the number of available allocations
	// (will later be multiplied by the number of frames)
	void setPoolSize(ui32 maxSets, std::unordered_map<vk::DescriptorType, ui32> const &sizes);

	void create(LogicalDevice *device, ui32 const &frameCount);
	void* get();
	void invalidate();

private:
	ui32 mMaxSets;
	std::unordered_map<vk::DescriptorType, ui32> mAvailableAllocationsPerType;
	vk::UniqueDescriptorPool mInternal;

};

NS_END
