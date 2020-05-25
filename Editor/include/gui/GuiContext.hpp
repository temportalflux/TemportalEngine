#pragma once

#include "TemportalEnginePCH.hpp"

#include "graphics/VulkanApi.hpp"
#include "graphics/Surface.hpp"
#include "graphics/PhysicalDevice.hpp"
#include "graphics/LogicalDevice.hpp"
#include "graphics/SwapChain.hpp"
#include "graphics/RenderPass.hpp"
#include "graphics/ImGuiFrame.hpp"

#include <vector>
#include <examples/imgui_impl_vulkan.h>

NS_GRAPHICS
class VulkanInstance;
NS_END

NS_GUI

class GuiContext : public graphics::VulkanApi
{

public:
	void initContext();
	void initWindow(void* handle);
	void initVulkan(graphics::VulkanInstance const *pInstance);
	void destroy(graphics::VulkanInstance const *pInstance);

private:
	ImGui_ImplVulkan_InitInfo mInfo;

	graphics::Surface mSurface;
	graphics::PhysicalDevice mPhysicalDevice;
	graphics::LogicalDevice mLogicalDevice;
	std::unordered_map<graphics::QueueFamily, vk::Queue> mQueues;
	vk::UniqueDescriptorPool mDescriptorPool;

	graphics::SwapChain mSwapChain;
	graphics::RenderPass mRenderPass;
	// Because IMGUI is "immediate", each frame needs to record its own command pool instructions
	std::vector<graphics::ImGuiFrame> mImGuiFrames;

	vk::UniqueDescriptorPool createDescriptorPool();

};

NS_END
