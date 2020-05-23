#pragma once

#include "dependency/SDL.hpp"
#include "types/integer.h"

#include <vulkan/vulkan.hpp>

class Editor
{

public:
	Editor();
	~Editor();

	void openWindow();
	void run();

private:

	dependency::SDL mDependencySDL[1];
	void* mpWindowHandle;

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

	void createWindow();
	void closeWindow();
	
	// VULKAN STUBBING - TO BE MOVED TO INTERNAL API
	void createSurface();
	void destroySurface();
	void setupVulkan();
	void cleanUpVulkan();
	void createFrameBuffers(i32 const width, i32 const height);

};
