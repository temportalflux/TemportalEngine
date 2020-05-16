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

	//typedef std::function<VkSurfaceKHR&(VkInstance const *pInstance)> FuncCreateSurface;
	typedef std::function<void(VkInstance const *pInstance, VkSurfaceKHR *pOutSurface)> FuncCreateSurface;

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

	vk::PhysicalDevice mPhysicalDevice;
	vk::UniqueDevice mLogicalDevice;
	
	vk::Queue mQueueGraphics;
	vk::Queue mQueuePresentation;

	vk::UniqueSurfaceKHR mSurface;
	
	vk::UniqueSwapchainKHR mSwapChain;
	std::vector<vk::Image> mSwapChainImages;
	vk::Extent2D mSwapChainResolution;
	vk::Format mSwapChainImageFormat;

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
	QueueFamilyIndicies findQueueFamilies(vk::PhysicalDevice const &device) const;
	
	std::optional<vk::UniqueDevice> createLogicalDevice(QueueFamilyIndicies const &queueFamilies, f32 const *graphicsQueuePriority);

	struct SwapChainSupport {
		vk::SurfaceCapabilitiesKHR capabilities;
		std::vector<vk::SurfaceFormatKHR> surfaceFormats;
		std::vector<vk::PresentModeKHR> presentationModes;
	};
	SwapChainSupport querySwapChainSupport(vk::PhysicalDevice const &device, vk::UniqueSurfaceKHR const &surface) const;
	vk::UniqueSwapchainKHR createSwapchain(vk::Extent2D &resolution, vk::Format &imageFormat);

public:
	Renderer(
		utility::SExecutableInfo const *const appInfo,
		utility::SExecutableInfo const *const engineInfo,
		ui32 width, ui32 height,
		std::vector<const char*> extensions,
		FuncCreateSurface createSurface
	);
	~Renderer();

};

NS_END

#pragma warning(pop)
#endif
