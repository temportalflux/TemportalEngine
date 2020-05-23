#pragma once

#include "dependency/SDL.hpp"
#include "types/integer.h"
#include "graphics/VulkanInstance.hpp"

#include <vulkan/vulkan.hpp>

class Editor
{

public:
	logging::LogSystem LogSystem;

	Editor();
	~Editor();

	void openWindow();
	void run();

private:
	dependency::SDL mDependencySDL[1];
	void* mpWindowHandle;

	graphics::VulkanInstance mVulkanInstance_new[1]; // TODO: rename

	vk::UniqueInstance mVulkanInstance;
	vk::PhysicalDevice mPhysicalDevice;
	ui32 mGraphicsQueueIndex;
	vk::UniqueDevice mLogicalDevice;
	vk::Queue mGraphicsQueue;
	vk::UniqueDescriptorPool mDescriptorPool;
	vk::UniqueSurfaceKHR mSurface;

	bool mIsRunning;

	bool initializeDependencies();
	void terminateDependencies();

	void closeWindow();

	void createWindow();
	void destroyWindow();

	std::vector<const char*> querySDLVulkanExtensions() const;
	void initializeVulkan();
	void destroyVulkan();
	
	// VULKAN STUBBING - TO BE MOVED TO INTERNAL API
	void createSurface();
	void destroySurface();
	void setupVulkan();
	void cleanUpVulkan();
	void createFrameBuffers(i32 const width, i32 const height);

};
