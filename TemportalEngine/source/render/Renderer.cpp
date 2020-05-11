#include "render/Renderer.hpp"
#include "Engine.hpp"
#include "ExecutableInfo.hpp"


using namespace render;
//using namespace vk;

VkDebugUtilsMessageSeverityFlagBitsEXT MIN_SEVERITY_TO_LOG = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
logging::ECategory LOG_CATEGORY = logging::ECategory::LOGINFO;
static VKAPI_ATTR ui32 VKAPI_CALL LogVulkanOutput(
	VkDebugUtilsMessageSeverityFlagBitsEXT severity,
	VkDebugUtilsMessageTypeFlagsEXT messageTypes,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData
)
{
	if (severity >= MIN_SEVERITY_TO_LOG)
	{
		LogRenderer(LOG_CATEGORY, pCallbackData->pMessage);
	}
	return VK_FALSE;
}

Renderer::Renderer(
	void* applicationHandle_win32, void* windowHandle_win32,
	utility::SExecutableInfo const *const appInfo,
	utility::SExecutableInfo const *const engineInfo,
	std::vector<const char*> extensions
)
	: maRequiredExtensionsSDL(extensions)
{

	// Extensions ---------------------------------------------------------------

	fetchAvailableExtensions();

	// Instance -----------------------------------------------------------------

	createInstance(appInfo, engineInfo);

#ifdef RENDERER_USE_VALIDATION_LAYERS
	setupVulkanMessenger();
#endif

	return;

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

std::vector<const char*> Renderer::getRequiredExtensions() const
{
	auto exts = maRequiredExtensionsSDL;
#ifdef RENDERER_USE_VALIDATION_LAYERS
	exts.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif
	return exts;
}

void Renderer::createInstance(utility::SExecutableInfo const *const appInfo, utility::SExecutableInfo const *const engineInfo)
{
	mpApplicationInfo->pApplicationName = appInfo->title;
	mpApplicationInfo->applicationVersion = appInfo->version;
	mpApplicationInfo->pEngineName = engineInfo->title;
	mpApplicationInfo->engineVersion = engineInfo->version;
	mpApplicationInfo->apiVersion = VK_API_VERSION_1_1;

	// One day perhaps use https://en.cppreference.com/w/cpp/utility/format/format in a macro to get the string equivalent of a SemanticVersion
	LogEngineInfo("Initializing Vulkan v%i.%i.%i with %s Application (v%i.%i.%i) on %s Engine (v%i.%i.%i)",
		TE_GET_MAJOR_VERSION(VK_API_VERSION_1_1), TE_GET_MINOR_VERSION(VK_API_VERSION_1_1), TE_GET_PATCH_VERSION(VK_API_VERSION_1_1),
		appInfo->title,
		TE_GET_MAJOR_VERSION(appInfo->version), TE_GET_MINOR_VERSION(appInfo->version), TE_GET_PATCH_VERSION(appInfo->version),
		engineInfo->title,
		TE_GET_MAJOR_VERSION(engineInfo->version), TE_GET_MINOR_VERSION(engineInfo->version), TE_GET_PATCH_VERSION(engineInfo->version)
	);

	mpInstanceInfo->pApplicationInfo = mpApplicationInfo;

	const auto requiredExtensions = this->getRequiredExtensions();
	mpInstanceInfo->setEnabledExtensionCount(requiredExtensions.size());
	mpInstanceInfo->setPpEnabledExtensionNames(requiredExtensions.data());

	if (checkValidationLayerSupport())
	{
		mpInstanceInfo->setEnabledLayerCount(mValidationLayers.size());
		mpInstanceInfo->setPpEnabledLayerNames(mValidationLayers.data());
	}
#ifdef RENDERER_USE_VALIDATION_LAYERS
	else
	{
		throw std::runtime_error("Vulkan validation layers requested, but there are none available.");
	}
#else
	else
	{
		mpInstanceInfo->setEnabledLayerCount(0);
	}
#endif

	// TODO: Route allocator callback through memory manager
	mpAppInstance = vk::createInstanceUnique(*mpInstanceInfo);
}

bool Renderer::checkValidationLayerSupport() const
{
#ifndef RENDERER_USE_VALIDATION_LAYERS
	return false;
#else
	for (const auto& availableLayer : vk::enumerateInstanceLayerProperties())
	{
		for (const auto& desiredLayer : mValidationLayers)
		{
			if (strcmp(availableLayer.layerName, desiredLayer) == 0)
			{
				return true;
			}
		}
	}
	return false;
#endif
}

void Renderer::setupVulkanMessenger()
{
	auto& instance = this->mpAppInstance.get();
	auto messenger = instance.createDebugUtilsMessengerEXTUnique(
		vk::DebugUtilsMessengerCreateInfoEXT{
			{},
			vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose
			| vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo
			| vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning
			| vk::DebugUtilsMessageSeverityFlagBitsEXT::eError,
			vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral
			| vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance
			| vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation,
			LogVulkanOutput
		},
		nullptr, vk::DispatchLoaderDynamic{ (VkInstance)instance, {} }
	);
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
