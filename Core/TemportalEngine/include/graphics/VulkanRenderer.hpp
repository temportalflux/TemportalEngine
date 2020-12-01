#pragma once

#include "TemportalEnginePCH.hpp"

#include "logging/Logger.hpp"
#include "graphics/types.hpp"
#include "graphics/LogicalDevice.hpp"
#include "graphics/LogicalDeviceInfo.hpp"
#include "graphics/PhysicalDevice.hpp"
#include "graphics/PhysicalDevicePreference.hpp"
#include "graphics/Surface.hpp"
#include "graphics/SwapChain.hpp"
#include "graphics/SwapChainInfo.hpp"
#include "graphics/ImageView.hpp"
#include "graphics/RenderPass.hpp"
#include "graphics/Buffer.hpp"
#include "graphics/Pipeline.hpp"
#include "graphics/FrameBuffer.hpp"
#include "graphics/CommandPool.hpp"
#include "graphics/CommandBuffer.hpp"
#include "graphics/Frame.hpp"
#include "graphics/GraphicsDevice.hpp"

#include <unordered_map>
#include <set>
#include <array>

NS_GRAPHICS
class VulkanInstance;
class PhysicalDevicePreference;
class ShaderModule;

class VulkanRenderer
{

public:
	VulkanRenderer();

	void setInstance(std::shared_ptr<VulkanInstance> pInstance);
	void takeOwnershipOfSurface(Surface &surface);

	void setPhysicalDevicePreference(PhysicalDevicePreference const &preference);
	void setLogicalDeviceInfo(LogicalDeviceInfo const &info);
	void setValidationLayers(std::vector<std::string> layers);
	void setSwapChainInfo(SwapChainInfo const &info);
	void setImageViewInfo(ImageViewInfo const &info);
	f32 getAspectRatio() const;

	virtual void initializeDevices();
	std::shared_ptr<GraphicsDevice> getDevice();
	
	// Creates a swap chain, and all objects that depend on it
	virtual void createRenderChain() = 0;

	virtual void finalizeInitialization() {}
	virtual void onInputEvent(void* evt) {}

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
	virtual void drawFrame();

	/**
	 * THREADED
	 * Called on the render thread to stall until idle.
	 */
	void waitUntilIdle();

	virtual void invalidate();

protected:
	std::shared_ptr<VulkanInstance> mpInstance;
	Surface mSurface;

#pragma region Devices, API, & Queues
	std::shared_ptr<GraphicsDevice> mpGraphicsDevice;
	PhysicalDevicePreference mPhysicalDevicePreference;
	LogicalDeviceInfo mLogicalDeviceInfo;
#pragma endregion

#pragma region Render Objects
	// if the render chain is out of date and needs to be recreated on next `update`
	// TODO: Might need a mutex if it can be written to by both render thread (vulkan callbacks) AND via `markRenderChainDirty`
	bool mbRenderChainDirty;
	SwapChainInfo mSwapChainInfo;
	SwapChain mSwapChain;
	ImageViewInfo mImageViewInfo;
	std::vector<ImageView> mFrameImageViews;
	// fence is active while a frame is drawing to this view
	std::vector<vk::Fence> mFrameImageFences;
#pragma endregion
	
	logging::Logger getLog() const;

	vk::Queue const& getQueue(EQueueFamily type) const;

	virtual void destroyRenderChain() = 0;
	void createSwapChain();
	void destroySwapChain();

	void createFrameImageViews();
	void destroyFrameImageViews();
	
	virtual void createRenderPass() = 0;
	virtual RenderPass* getRenderPass() = 0;
	virtual void destroyRenderPass() = 0;

	virtual void createFrames(uSize viewCount) = 0;
	virtual uSize getNumberOfFrames() const = 0;
	virtual graphics::Frame* getFrameAt(uSize idx) = 0;
	virtual void destroyFrames() = 0;

	bool acquireNextImage();
	virtual void prepareRender(ui32 idxCurrentFrame);
	virtual void render(graphics::Frame* frame, ui32 idxCurrentImage) = 0;
	bool present();
	virtual void onFramePresented(uIndex idxFrame) {}

private:

	uSize mIdxCurrentFrame;
	ui32 mIdxCurrentImage;

};

NS_END
