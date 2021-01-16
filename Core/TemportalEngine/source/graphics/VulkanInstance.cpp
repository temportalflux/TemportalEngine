#include "graphics/VulkanInstance.hpp"

#include "graphics/Surface.hpp"
#include "types/integer.h"
#include "utility/StringUtils.hpp"

#include <set>
#include <map>

using namespace graphics;

VulkanInstance::VulkanInstance()
	: mInstanceCreated(false)
	, mDebugMessenger(std::nullopt)
{
	mInfo.apiVersion = VK_API_VERSION_1_2;
	mCreateInfo.setPApplicationInfo(&mInfo);
	// Ensure validation layers are disabled
	setValidationLayers(std::nullopt);
}

ui32 VulkanInstance::apiVersion() const
{
	return this->mInfo.apiVersion;
}

void* VulkanInstance::get()
{
	return &this->mInstance.get();
}

VulkanInstance& VulkanInstance::createLogger(logging::LogSystem *logSys, bool bLogVulkanDebug)
{
	mUseVulkanDebugMessenger = bLogVulkanDebug;
	mLogger = logging::Logger("Vulkan", LOG_INFO, logSys);
	return *this;
}

logging::Logger VulkanInstance::getLog() const
{
	return mLogger;
}

VulkanInstance& VulkanInstance::setApplicationInfo(std::string const &name, Version const &version)
{
	assert(!mInstanceCreated);
	this->mInfoAppName = name;
	mInfo.setPApplicationName(this->mInfoAppName.c_str()).setApplicationVersion(version.packed);
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
	// Clear the set of extensions and load it will the passed extensions
	// Ensures there is only one of each type
	mEnabledExtensions.clear();
	mEnabledExtensions.insert(extensions.begin(), extensions.end());
	return *this;
}

std::unordered_set<char const*> const& VulkanInstance::enabledExtensions() const
{
	return this->mEnabledExtensions;
}

VulkanInstance& VulkanInstance::setValidationLayers(std::optional<std::vector<std::string>> layers)
{
	assert(!mInstanceCreated);
	// Check that all layers are actually available. Will be optimized out in a non-debug build
	if (layers.has_value())
	{
		std::set<std::string> desiredLayers(layers.value().begin(), layers.value().end());
		for (const auto& availableLayer : vk::enumerateInstanceLayerProperties())
		{
			for (auto iter = layers.value().begin(); iter != layers.value().end(); ++iter)
			{
				bool bIsDesiredLayer = strcmp(availableLayer.layerName, iter->c_str()) == 0;
				if (bIsDesiredLayer)
				{
					desiredLayers.erase(*iter);
				}
			}
		}
		// Assert-fail if there are unsupported validation layers
		assert(desiredLayers.empty());
		mValidationLayers = layers.value();
	}
	return *this;
}

bool VulkanInstance::isValid() const
{
	return mInstanceCreated;
}

void VulkanInstance::initialize()
{
	assert(!mInstanceCreated);

	// Append debug extensions as needed
	if (this->mUseVulkanDebugMessenger)
	{
		this->mEnabledExtensions.insert(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}

	// Copy the set back into a vector for sending to create info
	std::vector<char const*> extensionNames(this->mEnabledExtensions.begin(), this->mEnabledExtensions.end());
	mCreateInfo.setEnabledExtensionCount((ui32)extensionNames.size());
	mCreateInfo.setPpEnabledExtensionNames(extensionNames.data());

	auto validationLayerCount = (ui32)mValidationLayers.size();
	auto layers = utility::createTemporaryStringSet(this->mValidationLayers);
	mCreateInfo.setEnabledLayerCount(validationLayerCount);
	mCreateInfo.setPpEnabledLayerNames(validationLayerCount > 0 ? layers.data() : nullptr);

	auto appVersion = TE_GET_VERSION(mInfo.applicationVersion);
	auto engineVersion = TE_GET_VERSION(mInfo.engineVersion);
	getLog().log(LOG_INFO,
		"Initializing Vulkan v%i.%i.%i with %s Application (v%i.%i.%i) on %s Engine (v%i.%i.%i)",
		TE_GET_MAJOR_VERSION(mInfo.apiVersion), TE_GET_MINOR_VERSION(mInfo.apiVersion), TE_GET_PATCH_VERSION(mInfo.apiVersion),
		mInfo.pApplicationName, appVersion.unpacked.major, appVersion.unpacked.minor, appVersion.unpacked.patch,
		mInfo.pEngineName, engineVersion.unpacked.major, engineVersion.unpacked.minor, engineVersion.unpacked.patch
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
	mInstanceCreated = false;
}

std::optional<graphics::PhysicalDevice> VulkanInstance::pickPhysicalDevice(PhysicalDevicePreference & preference, Surface const *pSurface) const
{
	assert(isValid());

	std::multimap<ui32, graphics::PhysicalDevice> candidates;
	for (auto& device : this->mInstance->enumeratePhysicalDevices())
	{
		auto physicalDevice = PhysicalDevice(device, pSurface);
		auto score = preference.scoreDevice(&physicalDevice);
		if (score.has_value())
		{
			candidates.insert(std::make_pair(score.value(), physicalDevice));
		}
	}

	if (candidates.size() > 0)
	{
		return candidates.rbegin()->second;
	}
	return std::nullopt;
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
		vulkanInstance->getLog().log(LOG_INFO, pCallbackData->pMessage);
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
