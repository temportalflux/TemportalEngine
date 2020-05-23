#include "graphics/VulkanInstance.hpp"

#include "types/integer.h"
#include "version.h"

using namespace graphics;

VulkanInstance::VulkanInstance(logging::LogSystem *logSys, bool bLogVulkanDebug)
	: mInstanceCreated(false)
	, mLogger(logging::Logger("Vulkan", logSys))
	, mUseVulkanDebugMessenger(bLogVulkanDebug)
	, mDebugMessenger(std::nullopt)
{
	mInfo.apiVersion = VK_API_VERSION_1_2;
	mCreateInfo.setPApplicationInfo(&mInfo);
	// Ensure validation layers are disabled
	setValidationLayers(std::nullopt);
}

VulkanInstance& VulkanInstance::setApplicationInfo(utility::SExecutableInfo const &info)
{
	assert(!mInstanceCreated);
	mInfo.setPApplicationName(info.title).setApplicationVersion(info.version);
	return *this;
}

VulkanInstance& VulkanInstance::setEngineInfo(utility::SExecutableInfo const &info)
{
	assert(!mInstanceCreated);
	mInfo.setPEngineName(info.title).setEngineVersion(info.version);
	return *this;
}

VulkanInstance& VulkanInstance::setRequiredExtensions(std::vector<char const*> extensions)
{
	assert(!mInstanceCreated);
	mCreateInfo.setEnabledExtensionCount((ui32)extensions.size());
	mCreateInfo.setPpEnabledExtensionNames(extensions.data());
	return *this;
}

VulkanInstance& VulkanInstance::setValidationLayers(std::optional<std::vector<char const*>> layers)
{
	assert(!mInstanceCreated);
	// Check that all layers are actually available. Will be optimized out in a non-debug build
	if (layers.has_value())
	{
		for (const auto& availableLayer : vk::enumerateInstanceLayerProperties())
		{
			for (const auto& desiredLayerName : layers.value())
			{
				bool bHasLayer = strcmp(availableLayer.layerName, desiredLayerName) == 0;
				assert(bHasLayer);
			}
		}
	}

	mCreateInfo.setEnabledLayerCount(layers.has_value() ? (ui32)layers.value().size() : 0);
	mCreateInfo.setPpEnabledLayerNames(layers.has_value() ? layers.value().data() : nullptr);
	return *this;
}

void VulkanInstance::initialize()
{
	assert(!mInstanceCreated);

	getLog().log(logging::ECategory::LOGINFO,
		"Initializing Vulkan v%i.%i.%i with %s Application (v%i.%i.%i) on %s Engine (v%i.%i.%i)",
		TE_GET_MAJOR_VERSION(mInfo.apiVersion), TE_GET_MINOR_VERSION(mInfo.apiVersion), TE_GET_PATCH_VERSION(mInfo.apiVersion),
		mInfo.pApplicationName,
		TE_GET_MAJOR_VERSION(mInfo.applicationVersion), TE_GET_MINOR_VERSION(mInfo.applicationVersion), TE_GET_PATCH_VERSION(mInfo.applicationVersion),
		mInfo.pEngineName,
		TE_GET_MAJOR_VERSION(mInfo.engineVersion), TE_GET_MINOR_VERSION(mInfo.engineVersion), TE_GET_PATCH_VERSION(mInfo.engineVersion)
	);

	mInstance = vk::createInstanceUnique(mCreateInfo);
	mInstanceCreated = true;

	if (mUseVulkanDebugMessenger)
	{
		createDebugMessenger();
	}
}

void VulkanInstance::destroy()
{
	assert(mInstanceCreated);

	if (mDebugMessenger.has_value())
	{
		destroyDebugMessenger();
	}

	mInstance.reset();
}

static VKAPI_ATTR ui32 VKAPI_CALL LogVulkanOutput(
	VkDebugUtilsMessageSeverityFlagBitsEXT severity,
	VkDebugUtilsMessageTypeFlagsEXT messageTypes,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* ptrToVulkanInstance
)
{
#ifndef NDEBUG // TODO: Standardize flag in engine headers
	static auto severityThreashold = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
#else
	static auto severityThreashold = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT;
#endif

	auto vulkanInstance = reinterpret_cast<VulkanInstance*>(ptrToVulkanInstance);
	if (severity >= severityThreashold)
	{
		vulkanInstance->getLog().log(logging::ECategory::LOGINFO, pCallbackData->pMessage);
	}
	return VK_FALSE;
}

void VulkanInstance::createDebugMessenger()
{
	auto info = vk::DebugUtilsMessengerCreateInfoEXT(
		/*create flags*/ {},
		vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose
		| vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo
		| vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning
		| vk::DebugUtilsMessageSeverityFlagBitsEXT::eError,
		vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral
		| vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance
		| vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation,
		&LogVulkanOutput,
		/*pass wrapper object to static to capture logging*/ this
	);
	VkInstance rawInstance = (VkInstance)this->mInstance.get();
	vk::DispatchLoaderDynamic dldi(rawInstance, vkGetInstanceProcAddr);
	mDebugMessenger = this->mInstance->createDebugUtilsMessengerEXT(info, nullptr, dldi);
}

void VulkanInstance::destroyDebugMessenger()
{
	VkInstance rawInstance = (VkInstance)this->mInstance.get();
	vk::DispatchLoaderDynamic dldi(rawInstance, vkGetInstanceProcAddr);
	this->mInstance->destroyDebugUtilsMessengerEXT(mDebugMessenger.value(), nullptr, dldi);
	mDebugMessenger = std::nullopt;
}
