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
#include "graphics/Frame.hpp"

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
	void setShaders(std::set<ShaderModule*> const &shaders);
	void createRenderChain();

	/**
	 * Marks the destination of renders "dirty", meaning that the drawable size has changed.
	 * Will cause the render-chain to be re-created the next time `update` is called.
	 */
	void markRenderChainDirty();

	/**
	 * Called on the main thread to refresh any objects that need to be re-created.
	 */
	void update();

	/**
	 * THREADED
	 * Called on the render thread to submit & present a frame to the surface.
	 */
	void drawFrame();

	/**
	 * THREADED
	 * Called on the render thread to stall until idle.
	 */
	void waitUntilIdle();

	void invalidate();

private:
	VulkanInstance *mpInstance;
	Surface mSurface;

	PhysicalDevicePreference mPhysicalDevicePreference;
	PhysicalDevice mPhysicalDevice;
	
	LogicalDeviceInfo mLogicalDeviceInfo;
	LogicalDevice mLogicalDevice;
	std::unordered_map<QueueFamily, vk::Queue> mQueues;

	// if the render chain is out of date and needs to be recreated on next `update`
	// TODO: Might need a mutex if it can be written to by both render thread (vulkan callbacks) AND via `markRenderChainDirty`
	bool mbRenderChainDirty;
	SwapChainInfo mSwapChainInfo;
	SwapChain mSwapChain;
	std::vector<ImageView> mImageViews;
	RenderPass mRenderPass;

	std::vector<FrameBuffer> mFrameBuffers;
	Pipeline mPipeline;
	CommandPool mCommandPool;
	std::vector<CommandBuffer> mCommandBuffers;
	
	std::vector<Frame> mFrames;
	uSize mIdxCurrentFrame;
	ui32 mIdxCurrentImage;

	VulkanRenderer() = default;

	logging::Logger getLog() const;
	void pickPhysicalDevice();

	void destroyRenderChain();
	void createRenderObjects();
	void createCommandObjects();
	void recordCommandBufferInstructions();
	
	bool acquireNextImage();
	void prepareRender();
	void render();
	bool present();

};

NS_END
