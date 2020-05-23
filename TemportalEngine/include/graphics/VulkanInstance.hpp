#pragma once

#include "TemportalEnginePCH.hpp"

#include "ExecutableInfo.hpp"
#include "logging/Logger.hpp"

#include <optional>
#include <vector>
#include <vulkan/vulkan.hpp>

NS_GRAPHICS

/**
	Wrapper class for managing a vulkan instance.
	Only one of these should exist per application.
*/
class VulkanInstance
{

private:
	logging::Logger mLogger;

	bool mUseVulkanDebugMessenger;
	std::optional<vk::DebugUtilsMessengerEXT> mDebugMessenger;

	vk::ApplicationInfo mInfo;
	vk::InstanceCreateInfo mCreateInfo;
	bool mInstanceCreated;
	vk::UniqueInstance mInstance;

public:
	VulkanInstance(logging::LogSystem *logSys, bool bLogVulkanDebug);

	logging::Logger& getLog() { return mLogger; }

	VulkanInstance& setApplicationInfo(utility::SExecutableInfo const &info);
	VulkanInstance& setEngineInfo(utility::SExecutableInfo const &info);
	VulkanInstance& setRequiredExtensions(std::vector<char const*> extensions);
	VulkanInstance& setValidationLayers(std::optional<std::vector<char const*>> layers = std::nullopt);

	/**
		Creates the actual vulkan instances after everything has been configured.
		VulkanInstance should be considered constant until `destroy` is called.
	*/
	void initialize();

	/**
		Destroys the vulkan instance.
	*/
	void destroy();

private:

	void createDebugMessenger();
	void destroyDebugMessenger();

};

NS_END