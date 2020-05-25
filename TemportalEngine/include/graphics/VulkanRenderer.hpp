#pragma once

#include "TemportalEnginePCH.hpp"

#include "logging/Logger.hpp"
#include "graphics/LogicalDevice.hpp"
#include "graphics/LogicalDeviceInfo.hpp"
#include "graphics/PhysicalDevice.hpp"
#include "graphics/PhysicalDevicePreference.hpp"
#include "graphics/QueueFamily.hpp"
#include "graphics/Surface.hpp"
#include "graphics/SwapChain.hpp"
#include "graphics/SwapChainInfo.hpp"
#include "graphics/ImageView.hpp"
#include "graphics/RenderPass.hpp"
#include "graphics/Pipeline.hpp"
#include "graphics/FrameBuffer.hpp"
#include "graphics/CommandPool.hpp"
#include "graphics/CommandBuffer.hpp"

#include <unordered_map>
#include <set>

NS_GRAPHICS
class VulkanInstance;
class PhysicalDevicePreference;
class ShaderModule;

class VulkanRenderer
{

public:
	VulkanRenderer(VulkanInstance *pInstance, Surface &surface);

	void setPhysicalDevicePreference(PhysicalDevicePreference const &preference);
	void setLogicalDeviceInfo(LogicalDeviceInfo const &info);
	void setSwapChainInfo(SwapChainInfo const &info);

	void initializeDevices();
	// Creates a swap chain, and all objects that depend on it
	void constructRenderChain(std::set<ShaderModule*> const &shaders);

	void invalidate();

private:
	VulkanInstance *mpInstance;
	Surface mSurface;

	PhysicalDevicePreference mPhysicalDevicePreference;
	PhysicalDevice mPhysicalDevice;
	
	LogicalDeviceInfo mLogicalDeviceInfo;
	LogicalDevice mLogicalDevice;
	std::unordered_map<QueueFamily, vk::Queue> mQueues;

	SwapChainInfo mSwapChainInfo;
	SwapChain mSwapChain;
	std::vector<ImageView> mImageViews;
	RenderPass mRenderPass;
	std::vector<FrameBuffer> mFrameBuffers;
	Pipeline mPipeline;
	CommandPool mCommandPool;
	std::vector<CommandBuffer> mCommandBuffers;
	//std::vector<Frame> mFrames;

	VulkanRenderer() = default;

	logging::Logger getLog() const;
	void pickPhysicalDevice();
	void recordCommandBufferInstructions();

};

NS_END
