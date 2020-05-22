#ifndef TE_RENDERER_HPP
#define TE_RENDERER_HPP
#pragma warning(push)
#pragma warning(disable:4251) // disable STL warnings in dll

// PCH ------------------------------------------------------------------------
#include "TemportalEnginePCH.hpp"

// Libraries ------------------------------------------------------------------
#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.hpp>
#include <array>
#include <optional>
#include <functional>
#include <set>

// Engine ---------------------------------------------------------------------
#include "types/integer.h"
#include "input/Event.hpp"
#include "memory/SharedPtr.hpp"
//#include "math/pow.hpp"

NS_UTILITY
struct SExecutableInfo;
NS_END

#ifndef NDEBUG
#define RENDERER_USE_VALIDATION_LAYERS 1
#endif

NS_RENDER

class TEMPORTALENGINE_API Renderer
{
	typedef char const* CSTR;

public:

	typedef std::function<bool(VkInstance const *pInstance, VkSurfaceKHR *pOutSurface)> FuncCreateSurface;
	typedef std::vector<char> ShaderBinary;

public:
	Renderer(
		utility::SExecutableInfo const *const appInfo,
		utility::SExecutableInfo const *const engineInfo,
		ui32 width, ui32 height,
		std::vector<const char*> extensions,
		FuncCreateSurface createSurface
	);
	~Renderer();

	void drawFrame();
	void waitUntilIdle();

private:

#ifdef RENDERER_USE_VALIDATION_LAYERS
	const std::vector<const char*> mValidationLayers = {
		"VK_LAYER_KHRONOS_validation"
	};
#endif
	const std::vector<const char*> mDeviceExtensionNames = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};

	std::vector<CSTR> maRequiredExtensionsSDL;
	
	vk::ApplicationInfo mpApplicationInfo[1];
	vk::InstanceCreateInfo mpInstanceInfo[1];
	vk::UniqueInstance mAppInstance;
	vk::UniqueHandle<vk::DebugUtilsMessengerEXT, vk::DispatchLoaderDynamic> mDebugMessenger;

	std::optional<vk::PhysicalDevice> mPhysicalDevice;
	vk::UniqueDevice mLogicalDevice;
	
	vk::Queue mQueueGraphics;
	vk::Queue mQueuePresentation;

	vk::UniqueSurfaceKHR mSurface;
	
	vk::UniqueSwapchainKHR mSwapChain;
	vk::Extent2D mSwapChainResolution;
	vk::Format mSwapChainImageFormat;
	std::vector<vk::Image> mSwapChainImages;
	std::vector<vk::UniqueImageView> mSwapChainImageViews;

	vk::UniqueRenderPass mRenderPass;
	vk::UniquePipelineLayout mPipelineLayout;
	vk::UniquePipeline mPipeline;
	std::vector<vk::UniqueFramebuffer> mFrameBuffers;

	vk::UniqueCommandPool mCommandPool;
	std::vector<vk::UniqueCommandBuffer> mCommandBuffers;

	const ui8 MAX_FRAMES_IN_FLIGHT = 2;
	ui32 mCurrentFrame;
	std::vector<vk::UniqueSemaphore> mSemaphore_DrawPerFrame_ImageAvailable;
	std::vector<vk::UniqueSemaphore> mSemaphore_DrawPerFrame_RenderFinished;
	std::vector<vk::UniqueFence> mFencesInFlight;
	std::vector<VkFence> mImagesInFlight;

private:

	void fetchAvailableExtensions();
	std::vector<const char*> getRequiredExtensions() const;

	vk::UniqueInstance createInstance(utility::SExecutableInfo const *const appInfo, utility::SExecutableInfo const *const engineInfo);

	void setupVulkanMessenger();
	bool checkValidationLayerSupport() const;
	
	std::optional<vk::PhysicalDevice> pickPhysicalDevice();
	/**
		Returns a score for a physical device based on how suitable it is for this renderer.
		Return `std::nullopt` to indicate the device is not suitable at all.
	*/
	std::optional<ui8> getPhysicalDeviceSuitabilityScore(vk::PhysicalDevice const &device) const;

	bool checkDeviceExtensionSupport(vk::PhysicalDevice const &device) const;

	struct QueueFamilyIndicies
	{
		std::optional<ui32> idxGraphicsQueue;
		std::optional<ui32> idxPresentationQueue;

		bool hasFoundAllQueues() const
		{
			return idxGraphicsQueue.has_value() && idxPresentationQueue.has_value();
		}

		std::set<ui32> uniqueQueues() const
		{
			return { idxGraphicsQueue.value(), idxPresentationQueue.value() };
		}

		std::vector<ui32> allQueues() const
		{
			return { idxGraphicsQueue.value(), idxPresentationQueue.value() };
		}
	};
	QueueFamilyIndicies findQueueFamilies(std::optional<vk::PhysicalDevice> const &device) const;
	
	std::optional<vk::UniqueDevice> createLogicalDevice(QueueFamilyIndicies const &queueFamilies, f32 const *graphicsQueuePriority);

	struct SwapChainSupport {
		vk::SurfaceCapabilitiesKHR capabilities;
		std::vector<vk::SurfaceFormatKHR> surfaceFormats;
		std::vector<vk::PresentModeKHR> presentationModes;
	};
	SwapChainSupport querySwapChainSupport(std::optional<vk::PhysicalDevice> const &device, vk::UniqueSurfaceKHR const &surface) const;
	vk::UniqueSwapchainKHR createSwapchain(vk::Extent2D &resolution, vk::Format &imageFormat);
	void instantiateImageViews();

	std::optional<vk::UniqueShaderModule> createShaderModule(std::string const &filePath) const;
	vk::UniqueRenderPass createRenderPass();
	vk::UniquePipeline createGraphicsPipeline();
	void initializeFrameBuffers();

	vk::UniqueCommandPool createCommandPool() const;
	void initializeCommandBuffers();

	void createSyncObjects();

};

NS_END

#pragma warning(pop)
#endif
