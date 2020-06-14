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

#include <unordered_map>
#include <set>
#include <array>

NS_GRAPHICS
class VulkanInstance;
class PhysicalDevicePreference;
class ShaderModule;
class Uniform;

class VulkanRenderer
{

public:
	VulkanRenderer();

	void setInstance(VulkanInstance *pInstance);
	void takeOwnershipOfSurface(Surface &surface);

	void setPhysicalDevicePreference(PhysicalDevicePreference const &preference);
	void setLogicalDeviceInfo(LogicalDeviceInfo const &info);
	void setValidationLayers(std::vector<std::string> layers);
	void setSwapChainInfo(SwapChainInfo const &info);
	void setImageViewInfo(ImageViewInfo const &info);
	f32 getAspectRatio() const;

	virtual void initializeDevices();
	void addShader(std::shared_ptr<ShaderModule> shader);

	virtual void createInputBuffers(ui64 vertexBufferSize, ui64 indexBufferSize);
	void addUniform(std::shared_ptr<Uniform> uniform);

	template <typename TVertex>
	void writeVertexData(ui64 offset, std::vector<TVertex> verticies)
	{
		this->writeToBuffer(&this->mVertexBuffer, offset, (void*)verticies.data(), sizeof(TVertex) * verticies.size());
	}

	void writeIndexData(ui64 offset, std::vector<ui16> indicies)
	{
		this->mIndexBufferUnitType = vk::IndexType::eUint16;
		this->mIndexCount = (ui32)indicies.size();
		this->writeToBuffer(&this->mIndexBuffer, offset, (void*)indicies.data(), sizeof(ui16) * indicies.size());
	}

	template <ui64 IndexSize>
	void writeIndexData(ui64 offset, std::array<ui32, IndexSize> indicies)
	{
		this->mIndexBufferUnitType = vk::IndexType::eUint32;
		this->mIndexCount = (ui32)indicies.size();
		this->writeToBuffer(&this->mIndexBuffer, offset, (void*)indicies.data(), sizeof(ui32) * indicies.size());
	}
	
	// Creates a swap chain, and all objects that depend on it
	virtual void createRenderChain();

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
	VulkanInstance *mpInstance;
	Surface mSurface;

	PhysicalDevicePreference mPhysicalDevicePreference;
	PhysicalDevice mPhysicalDevice;
	
	LogicalDeviceInfo mLogicalDeviceInfo;
	std::unordered_map<QueueFamily::Enum, vk::Queue> mQueues;

	// if the render chain is out of date and needs to be recreated on next `update`
	// TODO: Might need a mutex if it can be written to by both render thread (vulkan callbacks) AND via `markRenderChainDirty`
	bool mbRenderChainDirty;
	SwapChainInfo mSwapChainInfo;
	SwapChain mSwapChain;
	ImageViewInfo mImageViewInfo;
	
	std::vector<ImageView> mImageViews;
	RenderPass mRenderPass;

	// TOOD: Create GameRenderer class which performs these operations instead of just overriding them

	CommandPool mCommandPoolTransient;
	ui32 mIndexCount;
	Buffer mVertexBuffer;
	Buffer mIndexBuffer;
	vk::IndexType mIndexBufferUnitType;

	std::vector<std::shared_ptr<Uniform>> mUniformPts;
	ui64 mTotalUniformSize;
	std::vector<Buffer> mUniformBuffers;
	vk::UniqueDescriptorPool mDescriptorPool;
	vk::UniqueDescriptorSetLayout mDescriptorLayout;
	std::vector<vk::DescriptorSet> mDescriptorSets;

	std::vector<FrameBuffer> mFrameBuffers;
	Pipeline mPipeline;
	CommandPool mCommandPool;
	std::vector<CommandBuffer> mCommandBuffers;
	
	uSize mIdxCurrentFrame;
	ui32 mIdxCurrentImage;
	
	logging::Logger getLog() const;
	void pickPhysicalDevice();
	virtual void destroyRenderChain();

	void writeToBuffer(Buffer* buffer, ui64 offset, void* data, ui64 size);
	void copyBetweenBuffers(Buffer *src, Buffer *dest, ui64 size);

	bool acquireNextImage();
	void prepareRender();
	virtual void render();
	bool present();

protected:

	// TODO: Move to private
	LogicalDevice mLogicalDevice;

	vk::Queue& getQueue(QueueFamily::Enum type);

	virtual void createRenderObjects();
	virtual void destroyRenderObjects();

	virtual void createUniformBuffers() = 0;
	virtual void destroyUniformBuffers() = 0;
	virtual void createDescriptorPool() = 0;
	virtual void destroyDescriptorPool() = 0;
	virtual void createCommandObjects() = 0;
	virtual void destroyCommandObjects() = 0;

	virtual void destroyInputBuffers();

	virtual void updateUniformBuffer(ui32 idxImageView);

	virtual void createFrames(uSize viewCount) = 0;
	virtual uSize getNumberOfFrames() const = 0;
	virtual graphics::Frame* getFrameAt(uSize idx) = 0;
	virtual void destroyFrames() = 0;

};

NS_END
