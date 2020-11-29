#include "graphics/vma/VMA.hpp"

#include "graphics/GraphicsDevice.hpp"
#include "graphics/PhysicalDevice.hpp"
#include "graphics/LogicalDevice.hpp"
#include "graphics/VulkanApi.hpp"
#include "graphics/VulkanInstance.hpp"

#define VMA_IMPLEMENTATION
#include "VulkanMemoryAllocator/src/vk_mem_alloc.h"

using namespace graphics;

inline VmaAllocator* asVma(void* ptr)
{
	return reinterpret_cast<VmaAllocator*>(ptr);
}

VulkanMemoryAllocator::VulkanMemoryAllocator(ui32 vulkanVersion, GraphicsDevice const* device)
{
	// https://gpuopen-librariesandsdks.github.io/VulkanMemoryAllocator/html/quick_start.html
	VmaAllocatorCreateInfo info = {};
	info.vulkanApiVersion = vulkanVersion;
	info.physicalDevice = graphics::extract<VkPhysicalDevice>(&device->physical());
	info.device = graphics::extract<VkDevice>(&device->logical());
	info.instance = graphics::extract<VkInstance>(device->instance().get());
	this->mInternal = malloc(sizeof(VmaAllocator));
	vmaCreateAllocator(&info, asVma(this->mInternal));
}

VulkanMemoryAllocator::~VulkanMemoryAllocator()
{
	vmaDestroyAllocator(*asVma(this->mInternal));
	free(this->mInternal);
	this->mInternal = nullptr;
}

AllocationInfo VulkanMemoryAllocator::getAllocationInfo(AllocationHandle handle) const
{
	VmaAllocationInfo info;
	vmaGetAllocationInfo(*asVma(this->mInternal), (VmaAllocation)handle, &info);
	return *reinterpret_cast<AllocationInfo*>(&info);
}

void* VulkanMemoryAllocator::mapMemory(AllocationHandle handle)
{
	void* ptr;
	vmaMapMemory(*asVma(this->mInternal), (VmaAllocation)handle, &ptr);
	return ptr;
}

void VulkanMemoryAllocator::unmapMemory(AllocationHandle handle)
{
	vmaUnmapMemory(*asVma(this->mInternal), (VmaAllocation)handle);
}

AllocationHandle VulkanMemoryAllocator::createBuffer(vk::BufferCreateInfo const &info, MemoryUsage usage, vk::Buffer &outBuffer)
{
	VmaAllocationCreateInfo allocInfo = {};
	allocInfo.usage = (VmaMemoryUsage)usage;

	VmaAllocation allocation;

	VkResult result = vmaCreateBuffer(
		*asVma(this->mInternal),
		reinterpret_cast<VkBufferCreateInfo const*>(&info), &allocInfo,
		reinterpret_cast<VkBuffer*>(&outBuffer),
		&allocation, nullptr
	);
	if (result == VK_SUCCESS)
	{
		return (AllocationHandle)allocation;
	}
	return {};
}

void VulkanMemoryAllocator::destroyBuffer(vk::Buffer &buffer, AllocationHandle handle)
{
	vmaDestroyBuffer(*asVma(this->mInternal), *reinterpret_cast<VkBuffer*>(&buffer), (VmaAllocation)handle);
}

AllocationHandle VulkanMemoryAllocator::createImage(vk::ImageCreateInfo const &info, MemoryUsage usage, vk::Image &outImage)
{
	VmaAllocationCreateInfo allocInfo;
	allocInfo.usage = (VmaMemoryUsage)usage;

	VmaAllocation allocation;

	VkResult result = vmaCreateImage(
		*asVma(this->mInternal),
		reinterpret_cast<VkImageCreateInfo const*>(&info), &allocInfo,
		reinterpret_cast<VkImage*>(&outImage),
		&allocation, nullptr
	);
	if (result == VK_SUCCESS)
	{
		return (AllocationHandle)allocation;
	}
	return {};
}

void VulkanMemoryAllocator::destroyImage(vk::Image &image, AllocationHandle handle)
{
	vmaDestroyImage(*asVma(this->mInternal), *reinterpret_cast<VkImage*>(&image), (VmaAllocation)handle);
}
