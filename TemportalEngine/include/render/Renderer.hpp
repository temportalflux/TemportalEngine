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

#define LogRenderer(cate, ...) DeclareLog("Renderer").log(cate, __VA_ARGS__);

NS_RENDER

class TEMPORTALENGINE_API Renderer
{
	typedef char const* CSTR;

private:

#ifdef RENDERER_USE_VALIDATION_LAYERS
	const std::vector<const char*> mValidationLayers = {
		"VK_LAYER_KHRONOS_validation"
	};
#endif
	
	vk::ApplicationInfo mpApplicationInfo[1];
	vk::InstanceCreateInfo mpInstanceInfo[1];
	vk::UniqueInstance mpAppInstance;

	std::vector<CSTR> maRequiredExtensionsSDL;

	std::optional<vk::PhysicalDevice> mPhysicalDevice;
	std::optional<size_t> mQueueFamilyIndex;
	vk::UniqueDevice mLogicalDevice;
	vk::Queue mQueue;
	vk::SurfaceKHR mSurface;
	vk::SwapchainKHR mSwapchain;

	void fetchAvailableExtensions();
	std::vector<const char*> getRequiredExtensions() const;
	void createInstance(utility::SExecutableInfo const *const appInfo, utility::SExecutableInfo const *const engineInfo);
	void setupVulkanMessenger();
	bool checkValidationLayerSupport() const;
	bool pickPhysicalDevice();
	void createLogicalDevice();
	void createSurface(void* applicationHandle_win32, void* windowHandle_win32);
	void createSwapchain();

	/*
	static uSize const MAX_PHYSICAL_DEVICE_COUNT = 4; // Max GPUs
	uSize mPhysicalDeviceCount;
	VkPhysicalDevice maVulkanPhysicalDevices[MAX_PHYSICAL_DEVICE_COUNT];
	//*/

	/*
	constexpr static uSize const MAX_EXTENSION_COUNT = 512;// pow<ui32, 2, 32>::value - 1;
	uSize mExtensionCount;
	VkExtensionProperties maVulkanAvailableExtensions[MAX_EXTENSION_COUNT];
	//*/

public:
	Renderer(
		void* applicationHandle_win32, void* windowHandle_win32,
		utility::SExecutableInfo const *const appInfo,
		utility::SExecutableInfo const *const engineInfo,
		std::vector<const char*> extensions
	);
	~Renderer();

	void initializeWindow();
	void render();

};

NS_END

#pragma warning(pop)
#endif
