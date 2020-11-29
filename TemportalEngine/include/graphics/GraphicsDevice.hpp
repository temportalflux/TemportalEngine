#pragma once

#include "TemportalEnginePCH.hpp"

#include "graphics/LogicalDevice.hpp"
#include "graphics/LogicalDeviceInfo.hpp"
#include "graphics/PhysicalDevice.hpp"
#include "graphics/PhysicalDevicePreference.hpp"
#include "graphics/CommandBuffer.hpp"

#include <vulkan/vulkan.hpp>

NS_GRAPHICS
class Buffer;
class CommandPool;
class DescriptorGroup;
class DescriptorPool;
class Image;
class Memory;
class Surface;
class SwapChain;
class VulkanInstance;
class VulkanMemoryAllocator;

/**
 * A wrapper class for bundling `PhysicalDevice` and `LogicalDevice`.
 */
class GraphicsDevice
{
	friend class Buffer;
	friend class CommandPool;
	friend class DescriptorGroup;
	friend class DescriptorPool;
	friend class Frame;
	friend class FrameBuffer;
	friend class Image;
	friend class ImageSampler;
	friend class ImageView;
	friend class Memory;
	friend class Pipeline;
	friend class RenderPass;
	friend class SwapChain;
	friend class ShaderModule;

public:
	GraphicsDevice() = default;
	GraphicsDevice(std::weak_ptr<VulkanInstance> instance);
	~GraphicsDevice();

	std::shared_ptr<VulkanInstance> instance() const;
	PhysicalDevice const& physical() const;
	PhysicalDevice& physical();
	LogicalDevice const& logical() const;
	LogicalDevice& logical();
	std::shared_ptr<VulkanMemoryAllocator> getVMA();

	void create(PhysicalDevicePreference prefs, LogicalDeviceInfo const &info, Surface const *pSurface);
	void destroy();

	vk::Queue const& getQueue(EQueueFamily type) const;

	QueueFamilyGroup queryQueueFamilyGroup() const;
	SwapChainSupport querySwapChainSupport() const;
	vk::PhysicalDeviceMemoryProperties getMemoryProperties() const;
	std::optional<vk::Format> pickFirstSupportedFormat(
		std::vector<vk::Format> const &candidates,
		vk::ImageTiling tiling, vk::FormatFeatureFlags flags
	) const;

private:
	std::weak_ptr<VulkanInstance> mpInstance;
	std::shared_ptr<VulkanMemoryAllocator> mpAllocator;

	PhysicalDevice mPhysicalDevice;
	LogicalDevice mLogicalDevice;
	std::unordered_map<EQueueFamily, vk::Queue> mQueues;

#pragma region Initializer Functions
	vk::UniqueDevice const& internalLogic() const;
#pragma region CommandPool
	vk::UniqueCommandPool createCommandPool(vk::CommandPoolCreateInfo const &info) const;
	std::vector<CommandBuffer> allocateCommandBuffers(CommandPool const *pool, vk::CommandBufferLevel const level, ui32 const count) const;
	void resetPool(CommandPool const *pool, vk::CommandPoolResetFlags flags = vk::CommandPoolResetFlags()) const;
#pragma endregion
#pragma region DescriptorGroup
	vk::UniqueDescriptorSetLayout createDescriptorSetLayout(vk::DescriptorSetLayoutCreateInfo const &info) const;
	std::vector<vk::DescriptorSet> allocateDescriptorSets(DescriptorPool const *pool, DescriptorGroup const *group, ui32 const count) const;
	void updateDescriptorSets(std::vector<vk::WriteDescriptorSet> writes, std::vector<vk::CopyDescriptorSet> copies = {}) const;
#pragma endregion
#pragma region DescriptorPool
	vk::UniqueDescriptorPool createDescriptorPool(vk::DescriptorPoolCreateInfo const &info) const;
#pragma endregion
#pragma region Fence
	vk::UniqueFence createFence(vk::FenceCreateInfo const &info = vk::FenceCreateInfo()) const;
	void waitForFences(std::vector<vk::Fence> fences, bool bWaitAll = true, ui64 timeout = UINT64_MAX) const;
	void resetFences(std::vector<vk::Fence> fences) const;
#pragma endregion
#pragma region FrameBuffer
	vk::UniqueFramebuffer createFrameBuffer(vk::FramebufferCreateInfo const &info) const;
#pragma endregion
#pragma region Image
	vk::UniqueImage createImage(vk::ImageCreateInfo const &info) const;
	vk::MemoryRequirements getMemoryRequirements(Image const *image) const;
#pragma endregion
#pragma region ImageView
	vk::UniqueImageView createImageView(vk::ImageViewCreateInfo const &info) const;
#pragma endregion
#pragma region Memory
	vk::UniqueDeviceMemory allocateMemory(vk::MemoryAllocateInfo const &info) const;
	void bindMemory(Memory const *memory, Image const *image, ui64 offset) const;
	void* mapMemory(Memory const *memory, ui64 offset, ui64 size) const;
	void unmapMemory(Memory const *memory) const;
#pragma endregion
#pragma region Pipeline
	vk::UniquePipelineLayout createPipelineLayout(vk::PipelineLayoutCreateInfo const &info) const;
	vk::UniquePipelineCache createPipelineCache(vk::PipelineCacheCreateInfo const &info) const;
	vk::UniquePipeline createPipeline(vk::UniquePipelineCache const &cache, vk::GraphicsPipelineCreateInfo const &info) const;
#pragma endregion
#pragma region RenderPass
	vk::UniqueRenderPass createRenderPass(vk::RenderPassCreateInfo const &info) const;
#pragma endregion
#pragma region Sampler
	vk::UniqueSampler createSampler(vk::SamplerCreateInfo const &info) const;
#pragma endregion
#pragma region Semaphore
	vk::UniqueSemaphore createSemaphore(vk::SemaphoreCreateInfo const &info = vk::SemaphoreCreateInfo()) const;
#pragma endregion
#pragma region Shader
	vk::UniqueShaderModule createShaderModule(vk::ShaderModuleCreateInfo const &info) const;
#pragma endregion
#pragma region SwapChain
	vk::UniqueSwapchainKHR createSwapchain(vk::SwapchainCreateInfoKHR const &info) const;
	std::vector<vk::Image> getSwapChainImages(SwapChain const *swapchain) const;
	vk::ResultValue<ui32> acquireNextImageIndex(
		SwapChain const *swapchain, ui64 timeout,
		std::optional<vk::Semaphore> waitForSemaphore, std::optional<vk::Fence> waitForFence
	);
#pragma endregion
#pragma endregion

};

NS_END
