#include "render/Renderer.hpp"
#include "Engine.hpp"
#include "ExecutableInfo.hpp"

using namespace render;
//using namespace vk;

Renderer::Renderer(
	void* applicationHandle_win32, void* windowHandle_win32,
	utility::SExecutableInfo const *const appInfo,
	utility::SExecutableInfo const *const engineInfo
)
	: maRequiredExtensionNames({
		"VK_KHR_surface",
		"VK_KHR_win32_surface",
		})
{

	// Extensions ---------------------------------------------------------------

	fetchAvailableExtensions();

	// Instance -----------------------------------------------------------------

	createInstance();

	// Physical Device ----------------------------------------------------------

	if (!pickPhysicalDevice())
	{
		return;
	}

	// Logical Device -----------------------------------------------------------

	createLogicalDevice();

	// Surface ------------------------------------------------------------------

	createSurface(applicationHandle_win32, windowHandle_win32);

	// Swap Chain ---------------------------------------------------------------

	createSwapchain();

	return;

	// Surface Capabilities -----------------------------------------------------

	/*

	// Command Pool -------------------------------------------------------------

	  vk::UniqueCommandPool commandPool = logicalDevice->createCommandPoolUnique(
	  vk::CommandPoolCreateInfo(vk::CommandPoolCreateFlags(), deviceQueueCreateInfo.queueFamilyIndex)
	);

	  std::vector<vk::UniqueCommandBuffer> commandBuffers = logicalDevice->allocateCommandBuffersUnique(
		  vk::CommandBufferAllocateInfo(commandPool.get(), vk::CommandBufferLevel::ePrimary, 1)
	  );
	//*/

}

Renderer::~Renderer()
{
}

void Renderer::fetchAvailableExtensions()
{
	auto availableExtensions = vk::enumerateInstanceExtensionProperties();
	LogEngineDebug("Available Vulkan extensions:");
	for (const auto& extension : availableExtensions)
	{
		LogEngineDebug("%s", extension.extensionName);
	}
}

void Renderer::createInstance()
{
	mpApplicationInfo->pApplicationName = "DemoGame";
	mpApplicationInfo->applicationVersion = 1;
	mpApplicationInfo->pEngineName = "TemportalEngine";
	mpApplicationInfo->engineVersion = 1;
	mpApplicationInfo->apiVersion = VK_API_VERSION_1_1;

	mpInstanceInfo->pApplicationInfo = mpApplicationInfo;
	mpInstanceInfo->setPpEnabledExtensionNames(maRequiredExtensionNames.data());
	mpAppInstance = vk::createInstanceUnique(*mpInstanceInfo);
}

bool Renderer::pickPhysicalDevice()
{
	mPhysicalDevice = std::nullopt;
	mQueueFamilyIndex = std::nullopt;

	auto physicalDevices = mpAppInstance->enumeratePhysicalDevices();

	if (physicalDevices.size() == 0) {
		LogEngine(logging::ECategory::LOGERROR, "Failed to find any GPUs with Vulkan support!");
		return false;
	}

	size_t deviceIndex = 0;
	mPhysicalDevice = std::make_optional(physicalDevices[deviceIndex]);

	std::vector<vk::QueueFamilyProperties> queueFamilyProperties = physicalDevices[deviceIndex].getQueueFamilyProperties();
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

	mQueueFamilyIndex = std::make_optional(graphicsQueueFamilyIndex);
	return true;
}

void Renderer::createLogicalDevice()
{
	float quePriorities[] = { 1.0f };
	vk::DeviceQueueCreateInfo deviceQueueCreateInfo(
		vk::DeviceQueueCreateFlags(),
		static_cast<ui32>(mQueueFamilyIndex.value()),
		1,
		quePriorities
	);

	const char *requiredGPUExtensions[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

	mLogicalDevice = mPhysicalDevice.value().createDeviceUnique(
		vk::DeviceCreateInfo()
		.setQueueCreateInfoCount(1)
		.setPQueueCreateInfos(&deviceQueueCreateInfo)
		.setEnabledExtensionCount(1)
		.setPpEnabledExtensionNames(requiredGPUExtensions)
	);

	mQueue = mLogicalDevice->getQueue(mQueueFamilyIndex.value(), 0);
}

void Renderer::createSurface(void* applicationHandle_win32, void* windowHandle_win32)
{
	mSurface = mpAppInstance->createWin32SurfaceKHR(
		vk::Win32SurfaceCreateInfoKHR()
		.setHinstance((HINSTANCE)applicationHandle_win32)
		.setHwnd((HWND)windowHandle_win32)
	);
}

void Renderer::createSwapchain()
{
	vk::SurfaceCapabilitiesKHR surfaceCapabilities =
		mPhysicalDevice->getSurfaceCapabilitiesKHR(mSurface);

	mSwapchain = mLogicalDevice->createSwapchainKHR(
		vk::SwapchainCreateInfoKHR(vk::SwapchainCreateFlagsKHR())
		// Surface info
		.setSurface(mSurface)
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
}

void Renderer::initializeWindow()
{

}

void Renderer::render()
{

}
