#pragma once

#include "TemportalEnginePCH.hpp"

#include "ExecutableInfo.hpp"
#include "logging/Logger.hpp"
#include "graphics/PhysicalDevicePreference.hpp"
#include "graphics/PhysicalDevice.hpp"

#include <optional>
#include <unordered_set>
#include <vector>
#include <vulkan/vulkan.hpp>

NS_GRAPHICS

/**
	Wrapper class for managing a vulkan instance.
	Only one of these should exist per application.
*/
class VulkanInstance
{
	friend class VulkanApi;
	friend class Surface;

public:
	VulkanInstance();

	VulkanInstance& createLogger(logging::LogSystem *logSys, bool bLogVulkanDebug);
	logging::Logger getLog() const;

	VulkanInstance& setApplicationInfo(utility::SExecutableInfo const &info);
	VulkanInstance& setEngineInfo(utility::SExecutableInfo const &info);
	VulkanInstance& setRequiredExtensions(std::vector<char const*> extensions);
	VulkanInstance& setValidationLayers(std::optional<std::vector<char const*>> layers = std::nullopt);
	std::vector<char const*> getValidationLayers() const;

	bool isValid() const;

	/**
		Creates the actual vulkan instances after everything has been configured.
		VulkanInstance should be considered constant until `destroy` is called.
	*/
	void initialize();

	/**
		Destroys the vulkan instance.
	*/
	void destroy();

	std::optional<graphics::PhysicalDevice> pickPhysicalDevice(PhysicalDevicePreference const &preference, Surface const *pSurface) const;


private:
	logging::Logger mLogger;

	bool mUseVulkanDebugMessenger;
	std::optional<vk::DebugUtilsMessengerEXT> mDebugMessenger;

	std::unordered_set<char const*> mEnabledExtensions;
	std::vector<char const*> mValidationLayers;

	vk::ApplicationInfo mInfo;
	vk::InstanceCreateInfo mCreateInfo;
	bool mInstanceCreated;
	vk::UniqueInstance mInstance;

	void createDebugMessenger();
	void destroyDebugMessenger();

};

NS_END