#include "render/Renderer.hpp"
#include "Engine.hpp"

using namespace render;
//using namespace vk;

Renderer::Renderer(void* applicationHandle_win32, void* windowHandle_win32)
	: maRequiredExtensionNames({
		"VK_KHR_surface",
		"VK_KHR_win32_surface",
	})
{

  // Extensions ---------------------------------------------------------------

  //auto extensionsAvailable = vk::enumerateInstanceExtensionProperties();

  // Instance -----------------------------------------------------------------

	mpApplicationInfo->pApplicationName = "DemoGame";
	mpApplicationInfo->applicationVersion = 1;
	mpApplicationInfo->pEngineName = "TemportalEngine";
	mpApplicationInfo->engineVersion = 1;
	mpApplicationInfo->apiVersion = VK_API_VERSION_1_1;

	mpInstanceInfo->pApplicationInfo = mpApplicationInfo;
	mpInstanceInfo->setPpEnabledExtensionNames(maRequiredExtensionNames.data());

	{
		auto appInstance = memory::NewUnique<vk::UniqueInstance>();
		assert(appInstance.has_value());
		mpAppInstance = appInstance.value();
	}
	*(mpAppInstance.GetRaw()) = vk::createInstanceUnique(*mpInstanceInfo);

  // Surface ------------------------------------------------------------------

  vk::SurfaceKHR surface = mpAppInstance->get().createWin32SurfaceKHR(
    vk::Win32SurfaceCreateInfoKHR()
      .setHinstance((HINSTANCE)applicationHandle_win32)
      .setHwnd((HWND)windowHandle_win32)
  );

  // Physical Device ----------------------------------------------------------

	auto layerProps = vk::enumerateInstanceLayerProperties();
	auto physicalDevices = mpAppInstance->get().enumeratePhysicalDevices();
	
	std::vector<vk::QueueFamilyProperties> queueFamilyProperties = physicalDevices[0].getQueueFamilyProperties();

	// Find the index of a queue family which supports graphics
	size_t graphicsQueueFamilyIndex = std::distance(
		queueFamilyProperties.begin(),
		// Find the iterator for a family property which supports graphics
		std::find_if(
			queueFamilyProperties.begin(),
			queueFamilyProperties.end(),
			[](vk::QueueFamilyProperties const &familyProps) {
				// Does the iterator have the graphics flag set
				return familyProps.queueFlags & vk::QueueFlagBits::eGraphics;
			}
		)
	);
	// Ensure that there is at least 1
	assert(graphicsQueueFamilyIndex < queueFamilyProperties.size());

  // Logical Device -----------------------------------------------------------

  float quePriorities[] = { 1.0f };
	vk::DeviceQueueCreateInfo deviceQueueCreateInfo(
		vk::DeviceQueueCreateFlags(),
		static_cast<ui32>(graphicsQueueFamilyIndex),
		1,
		quePriorities
	);

  const char *requiredGPUExtensions[] = { "VK_KHR_swapchain" };

	vk::UniqueDevice logicalDevice = physicalDevices[0].createDeviceUnique(
    vk::DeviceCreateInfo()
      .setQueueCreateInfoCount(1)
      .setPQueueCreateInfos(&deviceQueueCreateInfo)
      .setEnabledExtensionCount(1)
      .setPpEnabledExtensionNames(requiredGPUExtensions)
  );

  // Surface Capabilities -----------------------------------------------------

  vk::SurfaceCapabilitiesKHR surfaceCapabilities =
    physicalDevices[0].getSurfaceCapabilitiesKHR(surface);
  
  // Swap Chain ---------------------------------------------------------------

  vk::SwapchainKHR swapchain = logicalDevice.get().createSwapchainKHR(
    vk::SwapchainCreateInfoKHR(vk::SwapchainCreateFlagsKHR())
      // Surface info
      .setSurface(surface)
      // Double Buffering
      .setMinImageCount(2)
      // Image
      // size of window
      .setImageExtent(surfaceCapabilities.currentExtent)
      // Standard RGBA format
      .setImageFormat(vk::Format::eB8G8R8A8Unorm)
      // RGB, but non-linear to support HDR
      .setImageColorSpace(vk::ColorSpaceKHR::eSrgbNonlinear)
      // Unknown
      .setImageArrayLayers(1)
      .setImageUsage(vk::ImageUsageFlagBits::eColorAttachment)
      .setImageSharingMode(vk::SharingMode::eExclusive)
      .setPresentMode(vk::PresentModeKHR::eFifo)
  );

  // Command Pool -------------------------------------------------------------

	vk::UniqueCommandPool commandPool = logicalDevice->createCommandPoolUnique(
    vk::CommandPoolCreateInfo(vk::CommandPoolCreateFlags(), deviceQueueCreateInfo.queueFamilyIndex)
  );

	std::vector<vk::UniqueCommandBuffer> commandBuffers = logicalDevice->allocateCommandBuffersUnique(
		vk::CommandBufferAllocateInfo(commandPool.get(), vk::CommandBufferLevel::ePrimary, 1)
	);

}

Renderer::~Renderer()
{
}

void Renderer::initializeWindow()
{

}

void Renderer::render()
{

}
