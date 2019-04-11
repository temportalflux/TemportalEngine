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

NS_RENDER

class TEMPORTALENGINE_API Renderer
{
	typedef char const* CSTR;

private:

#ifdef NDEBUG
  const bool mUseValidationLayers = false;
#else
  const bool mUseValidationLayers = true;
#endif

	vk::ApplicationInfo mpApplicationInfo[1];
	vk::InstanceCreateInfo mpInstanceInfo[1];
	vk::UniqueInstance mpAppInstance;

	static ui8 const REQUIRED_EXTENSION_COUNT = 2;
	std::array<CSTR, REQUIRED_EXTENSION_COUNT> maRequiredExtensionNames;

  std::optional<vk::PhysicalDevice> mPhysicalDevice;
  std::optional<size_t> mQueueFamilyIndex;
  vk::UniqueDevice mLogicalDevice;
  vk::Queue mQueue;
  vk::SurfaceKHR mSurface;
  vk::SwapchainKHR mSwapchain;

  void fetchAvailableExtensions();
  void createInstance();
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
	Renderer(void* applicationHandle_win32, void* windowHandle_win32);
	~Renderer();

	void initializeWindow();
	void render();

};

NS_END

#pragma warning(pop)
#endif
