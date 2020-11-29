#pragma once

#include "TemportalEnginePCH.hpp"

#include <vulkan/vulkan.hpp>

NS_GRAPHICS
class GraphicsDevice;

enum class MemoryUsage : ui32
{
	eUnknown = 0,
	/** Memory will be used on device only, so fast access from the device is preferred.
	It usually means device-local GPU (video) memory.
	No need to be mappable on host.
	It is roughly equivalent of `D3D12_HEAP_TYPE_DEFAULT`.

	Usage:

	- Resources written and read by device, e.g. images used as attachments.
	- Resources transferred from host once (immutable) or infrequently and read by
		device multiple times, e.g. textures to be sampled, vertex buffers, uniform
		(constant) buffers, and majority of other types of resources used on GPU.

	Allocation may still end up in `HOST_VISIBLE` memory on some implementations.
	In such case, you are free to map it.
	You can use #VMA_ALLOCATION_CREATE_MAPPED_BIT with this usage type.
	*/
	eGPUOnly = 1,
	/** Memory will be mappable on host.
	It usually means CPU (system) memory.
	Guarantees to be `HOST_VISIBLE` and `HOST_COHERENT`.
	CPU access is typically uncached. Writes may be write-combined.
	Resources created in this pool may still be accessible to the device, but access to them can be slow.
	It is roughly equivalent of `D3D12_HEAP_TYPE_UPLOAD`.

	Usage: Staging copy of resources used as transfer source.
	*/
	eCPUSource = 2,
	/**
	Memory that is both mappable on host (guarantees to be `HOST_VISIBLE`) and preferably fast to access by GPU.
	CPU access is typically uncached. Writes may be write-combined.

	Usage: Resources written frequently by host (dynamic), read by device. E.g. textures (with LINEAR layout), vertex buffers, uniform buffers updated every frame or every draw call.
	*/
	eCPUToGPU = 3,
	/** Memory mappable on host (guarantees to be `HOST_VISIBLE`) and cached.
	It is roughly equivalent of `D3D12_HEAP_TYPE_READBACK`.

	Usage:

	- Resources written by device, read by host - results of some computations, e.g. screen capture, average scene luminance for HDR tone mapping.
	- Any resources read or accessed randomly on host, e.g. CPU-side copy of vertex buffer used as source of transfer, but also used for collision detection.
	*/
	eGPUToCPU = 4,
	/** CPU memory - memory that is preferably not `DEVICE_LOCAL`, but also not guaranteed to be `HOST_VISIBLE`.

	Usage: Staging copy of resources moved from GPU memory to CPU memory as part
	of custom paging/residency mechanism, to be moved back to GPU memory when needed.
	*/
	eCPUCopy = 5,
	/** Lazily allocated GPU memory having `VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT`.
	Exists mostly on mobile platforms. Using it on desktop PC or other GPUs with no such memory type present will fail the allocation.

	Usage: Memory for transient attachment images (color attachments, depth attachments etc.), created with `VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT`.

	Allocations with this usage are always created as dedicated - it implies #VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT.
	*/
	eGPULazy = 6,
};

typedef void* AllocationHandle;

struct AllocationInfo
{
	ui32 memoryType;
	vk::DeviceMemory* deviceMemory;
	uIndex offset;
	uSize size;
	void* cpuMapping;
	void* userData;
};

class VulkanMemoryAllocator
{
public:
	VulkanMemoryAllocator(ui32 vulkanVersion, GraphicsDevice const* device);
	~VulkanMemoryAllocator();

	AllocationHandle createBuffer(vk::BufferCreateInfo const &info, MemoryUsage usage, vk::Buffer &outBuffer);
	void destroyBuffer(vk::Buffer &buffer, AllocationHandle handle);
	AllocationHandle createImage(vk::ImageCreateInfo const &info, MemoryUsage usage, vk::Image &outImage);
	void destroyImage(vk::Image &image, AllocationHandle handle);

	AllocationInfo getAllocationInfo(AllocationHandle handle) const;
	void* mapMemory(AllocationHandle handle);
	void unmapMemory(AllocationHandle handle);

private:
	void* mInternal;

};

NS_END
