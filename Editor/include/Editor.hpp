#pragma once

#include "dependency/SDL.hpp"
#include "types/integer.h"
#include "graphics/VulkanInstance.hpp"
#include "graphics/Surface.hpp"
#include "graphics/PhysicalDevice.hpp"
#include "graphics/LogicalDevice.hpp"
#include "graphics/SwapChain.hpp"
#include "graphics/RenderPass.hpp"
#include "graphics/ImGuiFrame.hpp"

#include <unordered_map>
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

	static std::vector<const char*> VulkanValidationLayers;
	graphics::VulkanInstance mVulkanInstance;
	graphics::Surface mSurface;
	graphics::PhysicalDevice mPhysicalDevice;
	graphics::LogicalDevice mLogicalDevice;
	std::unordered_map<graphics::QueueFamily, vk::Queue> mQueues;
	vk::UniqueDescriptorPool mDescriptorPool;
	
	graphics::SwapChain mSwapChain;
	graphics::RenderPass mRenderPass;
	// Because IMGUI is "immediate", each frame needs to record its own command pool instructions
	std::vector<graphics::ImGuiFrame> mImGuiFrames;

	bool mIsRunning;

	bool initializeDependencies();
	void terminateDependencies();

	void createWindow();
	void destroyWindow();

	std::vector<const char*> querySDLVulkanExtensions() const;
	void initializeVulkan();
	vk::UniqueDescriptorPool createDescriptorPool();

	// VULKAN STUBBING - TO BE MOVED TO INTERNAL API
	void createFrameBuffers(i32 const width, i32 const height);

};
