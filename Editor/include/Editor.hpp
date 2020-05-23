#pragma once

#include "dependency/SDL.hpp"
#include "types/integer.h"
#include "graphics/VulkanInstance.hpp"
#include "graphics/Surface.hpp"
#include "graphics/PhysicalDevice.hpp"

#include <vulkan/vulkan.hpp>

class Editor
{

public:
	logging::LogSystem LogSystem;

	Editor();
	~Editor();

	void openWindow();
	void closeWindow();
	void run();

private:
	dependency::SDL mDependencySDL[1];
	void* mpWindowHandle;

	graphics::VulkanInstance mVulkanInstance;
	graphics::Surface mSurface;
	graphics::PhysicalDevice mPhysicalDevice;

	ui32 mGraphicsQueueIndex;
	vk::UniqueDevice mLogicalDevice;
	vk::Queue mGraphicsQueue;
	vk::UniqueDescriptorPool mDescriptorPool;

	bool mIsRunning;

	bool initializeDependencies();
	void terminateDependencies();

	void createWindow();
	void destroyWindow();

	std::vector<const char*> querySDLVulkanExtensions() const;
	void initializeVulkan();
	
	// VULKAN STUBBING - TO BE MOVED TO INTERNAL API
	void setupVulkan();
	void cleanUpVulkan();
	void createFrameBuffers(i32 const width, i32 const height);

};
