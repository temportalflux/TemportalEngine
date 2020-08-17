#include "graphics/GraphicsDevice.hpp"

#include "graphics/Buffer.hpp"
#include "graphics/CommandPool.hpp"
#include "graphics/DescriptorGroup.hpp"
#include "graphics/DescriptorPool.hpp"
#include "graphics/Image.hpp"
#include "graphics/Memory.hpp"
#include "graphics/SwapChain.hpp"
#include "graphics/VulkanInstance.hpp"

using namespace graphics;

GraphicsDevice::GraphicsDevice(std::weak_ptr<VulkanInstance> instance)
	: mpInstance(instance)
{
}

GraphicsDevice::~GraphicsDevice()
{
	this->destroy();
}

PhysicalDevice& GraphicsDevice::physical()
{
	return this->mPhysicalDevice;
}

LogicalDevice& GraphicsDevice::logical()
{
	return this->mLogicalDevice;
}

vk::UniqueDevice const& GraphicsDevice::internalLogic() const
{
	return this->mLogicalDevice.mInternal;
}

void GraphicsDevice::create(PhysicalDevicePreference prefs, LogicalDeviceInfo const &info, Surface const *pSurface)
{
	auto instance = this->mpInstance.lock();
	auto optPhysicalDevice = instance->pickPhysicalDevice(prefs, pSurface);
	if (!optPhysicalDevice)
	{
		instance->getLog().log(logging::ECategory::LOGERROR, "Failed to find a suitable GPU/physical device.");
		return;
	}
	this->mPhysicalDevice = *optPhysicalDevice;

	this->mLogicalDevice = this->mPhysicalDevice.createLogicalDevice(&info, &prefs);
	this->mQueues = this->mLogicalDevice.findQueues(info.getQueues());

	auto const physicalProps = this->mPhysicalDevice.getProperties();
	auto const apiVersion = TE_GET_VERSION(physicalProps.apiVersion).toString();
	auto const driverVersion = TE_GET_VERSION(physicalProps.driverVersion).toString();
	instance->getLog().log(
		LOG_INFO, "Created graphics devices:\n\t%s named %s (id=%i api=%s driver=%s)\n\tMax Allocation Count: %i\n\tMax Size Per Allocation: ???",
		vk::to_string(physicalProps.deviceType).c_str(),
		physicalProps.deviceName,
		physicalProps.deviceID,
		apiVersion.c_str(), driverVersion.c_str(),
		physicalProps.limits.maxMemoryAllocationCount
	);

}

void GraphicsDevice::destroy()
{
	this->mQueues.clear();
	this->mLogicalDevice.invalidate();
	this->mPhysicalDevice.invalidate();
}

vk::Queue const& GraphicsDevice::getQueue(QueueFamily::Enum type) const
{
	return this->mQueues.find(type)->second;
}

QueueFamilyGroup GraphicsDevice::queryQueueFamilyGroup() const
{
	return this->mPhysicalDevice.queryQueueFamilyGroup();
}

SwapChainSupport GraphicsDevice::querySwapChainSupport() const
{
	return this->mPhysicalDevice.querySwapChainSupport();
}

vk::PhysicalDeviceMemoryProperties GraphicsDevice::getMemoryProperties() const
{
	return this->mPhysicalDevice.getMemoryProperties();
}

std::optional<vk::Format> GraphicsDevice::pickFirstSupportedFormat(
	std::vector<vk::Format> const &candidates,
	vk::ImageTiling tiling, vk::FormatFeatureFlags flags
) const
{
	return this->mPhysicalDevice.pickFirstSupportedFormat(candidates, tiling, flags);
}

#pragma region Initializer Functions

#pragma region Buffer

vk::UniqueBuffer GraphicsDevice::createBuffer(vk::BufferCreateInfo const &info) const
{
	return this->internalLogic()->createBufferUnique(info);
}

vk::MemoryRequirements GraphicsDevice::getMemoryRequirements(Buffer const *buffer) const
{
	return this->internalLogic()->getBufferMemoryRequirements(buffer->mInternal.get());
}

#pragma endregion

#pragma region CommandPool

vk::UniqueCommandPool GraphicsDevice::createCommandPool(vk::CommandPoolCreateInfo const &info) const
{
	return this->internalLogic()->createCommandPoolUnique(info);
}

std::vector<CommandBuffer> GraphicsDevice::allocateCommandBuffers(CommandPool const *pool, vk::CommandBufferLevel const level, ui32 const count) const
{
	auto bufferHandles = this->internalLogic()->allocateCommandBuffersUnique(
		vk::CommandBufferAllocateInfo()
		.setLevel(vk::CommandBufferLevel::ePrimary)
		.setCommandPool(pool->mInternal.get())
		.setCommandBufferCount((ui32)count)
	);
	auto buffers = std::vector<CommandBuffer>(count);
	for (uSize i = 0; i < count; ++i)
	{
		buffers[i].mInternal.swap(bufferHandles[i]);
	}
	return buffers;
}

void GraphicsDevice::resetPool(CommandPool const *pool, vk::CommandPoolResetFlags flags /*= vk::CommandPoolResetFlags()*/) const
{
	this->internalLogic()->resetCommandPool(pool->mInternal.get(), flags);
}

#pragma endregion

#pragma region DescriptorGroup

vk::UniqueDescriptorSetLayout GraphicsDevice::createDescriptorSetLayout(vk::DescriptorSetLayoutCreateInfo const &info) const
{
	return this->internalLogic()->createDescriptorSetLayoutUnique(info);
}

std::vector<vk::DescriptorSet> GraphicsDevice::allocateDescriptorSets(DescriptorPool const *pool, DescriptorGroup const *group, ui32 const count) const
{
	std::vector<vk::DescriptorSetLayout> layouts(count, group->mInternalLayout.get());
	return this->internalLogic()->allocateDescriptorSets(
		vk::DescriptorSetAllocateInfo()
		.setDescriptorPool(pool->mInternal.get())
		.setDescriptorSetCount(count)
		.setPSetLayouts(layouts.data())
	);
}

void GraphicsDevice::updateDescriptorSets(std::vector<vk::WriteDescriptorSet> writes, std::vector<vk::CopyDescriptorSet> copies) const
{
	return this->internalLogic()->updateDescriptorSets(writes, copies);
}

#pragma endregion

#pragma region DescriptorPool

vk::UniqueDescriptorPool GraphicsDevice::createDescriptorPool(vk::DescriptorPoolCreateInfo const &info) const
{
	return this->internalLogic()->createDescriptorPoolUnique(info);
}

#pragma endregion

#pragma region Fence

vk::UniqueFence GraphicsDevice::createFence(vk::FenceCreateInfo const &info /*= vk::FenceCreateInfo()*/) const
{
	return this->internalLogic()->createFenceUnique(info);
}

void GraphicsDevice::waitForFences(std::vector<vk::Fence> fences, bool bWaitAll /*= true*/, ui64 timeout /*= UINT64_MAX*/) const
{
	this->internalLogic()->waitForFences(fences, bWaitAll, timeout);
}

void GraphicsDevice::resetFences(std::vector<vk::Fence> fences) const
{
	this->internalLogic()->resetFences(fences);
}

#pragma endregion

#pragma region FrameBuffer

vk::UniqueFramebuffer GraphicsDevice::createFrameBuffer(vk::FramebufferCreateInfo const &info) const
{
	return this->internalLogic()->createFramebufferUnique(info);
}

#pragma endregion

#pragma region Image

vk::UniqueImage GraphicsDevice::createImage(vk::ImageCreateInfo const &info) const
{
	return this->internalLogic()->createImageUnique(info);
}

vk::MemoryRequirements GraphicsDevice::getMemoryRequirements(Image const *image) const
{
	return this->internalLogic()->getImageMemoryRequirements(image->mInternal.get());
}

#pragma endregion

#pragma region ImageView

vk::UniqueImageView GraphicsDevice::createImageView(vk::ImageViewCreateInfo const &info) const
{
	return this->internalLogic()->createImageViewUnique(info);
}

#pragma endregion

#pragma region Memory

vk::UniqueDeviceMemory GraphicsDevice::allocateMemory(vk::MemoryAllocateInfo const &info) const
{
	return this->internalLogic()->allocateMemoryUnique(info);
}

void GraphicsDevice::bindMemory(Memory const *memory, Buffer const *buffer, ui64 offset) const
{
	this->internalLogic()->bindBufferMemory(buffer->mInternal.get(), memory->mInternal.get(), offset);
}

void GraphicsDevice::bindMemory(Memory const *memory, Image const *image, ui64 offset) const
{
	this->internalLogic()->bindImageMemory(image->mInternal.get(), memory->mInternal.get(), offset);
}

void* GraphicsDevice::mapMemory(Memory const *memory, ui64 offset, ui64 size) const
{
	return this->internalLogic()->mapMemory(memory->mInternal.get(), offset, size, vk::MemoryMapFlags());
}

void GraphicsDevice::unmapMemory(Memory const *memory) const
{
	this->internalLogic()->unmapMemory(memory->mInternal.get());
}


#pragma endregion

#pragma region Pipeline

vk::UniquePipelineLayout GraphicsDevice::createPipelineLayout(vk::PipelineLayoutCreateInfo const &info) const
{
	return this->internalLogic()->createPipelineLayoutUnique(info);
}

vk::UniquePipelineCache GraphicsDevice::createPipelineCache(vk::PipelineCacheCreateInfo const &info) const
{
	return this->internalLogic()->createPipelineCacheUnique(info);
}

vk::UniquePipeline GraphicsDevice::createPipeline(vk::UniquePipelineCache const &cache, vk::GraphicsPipelineCreateInfo const &info) const
{
	return this->internalLogic()->createGraphicsPipelineUnique(cache.get(), info);
}

#pragma endregion

#pragma region RenderPass

vk::UniqueRenderPass GraphicsDevice::createRenderPass(vk::RenderPassCreateInfo const &info) const
{
	return this->internalLogic()->createRenderPassUnique(info);
}

#pragma endregion

#pragma region Sampler

vk::UniqueSampler GraphicsDevice::createSampler(vk::SamplerCreateInfo const &info) const
{
	return this->internalLogic()->createSamplerUnique(info);
}

#pragma endregion

#pragma region Semaphore

vk::UniqueSemaphore GraphicsDevice::createSemaphore(vk::SemaphoreCreateInfo const &info /*= vk::SemaphoreCreateInfo()*/) const
{
	return this->internalLogic()->createSemaphoreUnique(info);
}

#pragma endregion

#pragma region Shader

vk::UniqueShaderModule GraphicsDevice::createShaderModule(vk::ShaderModuleCreateInfo const &info) const
{
	return this->internalLogic()->createShaderModuleUnique(info);
}

#pragma endregion

#pragma region SwapChain

vk::UniqueSwapchainKHR GraphicsDevice::createSwapchain(vk::SwapchainCreateInfoKHR const &info) const
{
	return this->internalLogic()->createSwapchainKHRUnique(info);
}

std::vector<vk::Image> GraphicsDevice::getSwapChainImages(SwapChain const *swapchain) const
{
	return this->internalLogic()->getSwapchainImagesKHR(swapchain->mInternal.get());
}

vk::ResultValue<ui32> GraphicsDevice::acquireNextImageIndex(
	SwapChain const *swapchain, ui64 timeout,
	std::optional<vk::Semaphore> waitForSemaphore, std::optional<vk::Fence> waitForFence
)
{
	return this->internalLogic()->acquireNextImageKHR(
		swapchain->mInternal.get(), timeout,
		waitForSemaphore ? *waitForSemaphore : vk::Semaphore(),
		waitForFence ? *waitForFence : vk::Fence()
	);
}

#pragma endregion

#pragma endregion
