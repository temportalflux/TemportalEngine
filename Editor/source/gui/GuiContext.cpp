#include "gui/GuiContext.hpp"

#include "graphics/VulkanInstance.hpp"
#include "graphics/PhysicalDevicePreference.hpp"
#include "graphics/LogicalDeviceInfo.hpp"
#include "graphics/SwapChainInfo.hpp"

#include <imgui.h>
#include <examples/imgui_impl_sdl.h>

using namespace gui;

void GuiContext::initContext()
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	ImGui::StyleColorsDark();
}

void GuiContext::initWindow(void* handle)
{
	ImGui_ImplSDL2_InitForVulkan(reinterpret_cast<SDL_Window*>(handle));
	mSurface = graphics::Surface(handle);
}

void GuiContext::initVulkan(graphics::VulkanInstance const *pInstance)
{
	mSurface.initialize(pInstance);

	auto optPhysicalDevice = pInstance->pickPhysicalDevice(
		graphics::PhysicalDevicePreference()
		.addCriteriaQueueFamily(graphics::QueueFamily::eGraphics),
		&mSurface
	);
	if (!optPhysicalDevice.has_value())
	{
		pInstance->getLog().log(logging::ECategory::LOGERROR, "Failed to find a suitable GPU/physical device.");
		return;
	}
	mPhysicalDevice = optPhysicalDevice.value();

	static const auto logicalDeviceInfo = graphics::LogicalDeviceInfo()
		.addQueueFamily(graphics::QueueFamily::eGraphics)
		.addDeviceExtension(VK_KHR_SWAPCHAIN_EXTENSION_NAME)
		.setValidationLayers(pInstance->getValidationLayers());
	mLogicalDevice = mPhysicalDevice.createLogicalDevice(&logicalDeviceInfo);

	mQueues = mLogicalDevice.findQueues({ graphics::QueueFamily::eGraphics });

	mDescriptorPool = createDescriptorPool();

	auto queueFamilyGroup = mPhysicalDevice.queryQueueFamilyGroup();
	mSwapChain
		.setSupport(mPhysicalDevice.querySwapChainSupport())
		.setQueueFamilyGroup(queueFamilyGroup)
		.setInfo(graphics::SwapChainInfo()
			.addFormatPreference(vk::Format::eB8G8R8A8Unorm)
			.addFormatPreference(vk::Format::eR8G8B8A8Unorm)
			.addFormatPreference(vk::Format::eB8G8R8Unorm)
			.addFormatPreference(vk::Format::eR8G8B8Unorm)
			.setColorSpace(vk::ColorSpaceKHR::eSrgbNonlinear)
#ifdef IMGUI_UNLIMITED_FRAME_RATE
			.addPresentModePreference(vk::PresentModeKHR::eMailbox)
			.addPresentModePreference(vk::PresentModeKHR::eImmediate)
#endif
			.addPresentModePreference(vk::PresentModeKHR::eFifo)
		)
		.create(&mLogicalDevice, &mSurface);

	mRenderPass.initFromSwapChain(&mSwapChain).create(&mLogicalDevice);

	mImageViews = mSwapChain.createImageViews({
		vk::ImageViewType::e2D,
		{
			vk::ComponentSwizzle::eR,
			vk::ComponentSwizzle::eG,
			vk::ComponentSwizzle::eB,
			vk::ComponentSwizzle::eA
		}
	});

	auto viewCount = mImageViews.size();
	mDynamicFrames.resize(viewCount);
	for (uSize i = 0; i < viewCount; ++i)
	{
		mDynamicFrames[i]
			.setRenderPass(&mRenderPass)
			.setView(&mImageViews[i])
			.setQueueFamilyGroup(queueFamilyGroup)
			.create(&mLogicalDevice);
	}

	mImagesInFlight.resize(viewCount);

	mInfo.Instance = this->extract(pInstance);
	mInfo.PhysicalDevice = this->extract(&mPhysicalDevice);
	mInfo.Device = this->extract(&mLogicalDevice);
	mInfo.QueueFamily = queueFamilyGroup.getQueueIndex(graphics::QueueFamily::eGraphics).value();
	mInfo.Queue = (VkQueue)mQueues[graphics::QueueFamily::eGraphics];
	mInfo.PipelineCache = VK_NULL_HANDLE;
	mInfo.DescriptorPool = (VkDescriptorPool)mDescriptorPool.get();
	mInfo.Allocator = nullptr;
	mInfo.MinImageCount = (ui32)viewCount;
	mInfo.ImageCount = (ui32)viewCount;
	mInfo.CheckVkResultFn = nullptr;
	ImGui_ImplVulkan_Init(&mInfo, this->extract(&mRenderPass));
}

vk::UniqueDescriptorPool GuiContext::createDescriptorPool()
{
	ui32 const poolSize = 1000;
	std::vector<vk::DescriptorType> poolTypes = {
		vk::DescriptorType::eSampler,
		vk::DescriptorType::eCombinedImageSampler,
		vk::DescriptorType::eSampledImage,
		vk::DescriptorType::eStorageImage,
		vk::DescriptorType::eUniformTexelBuffer,
		vk::DescriptorType::eStorageTexelBuffer,
		vk::DescriptorType::eUniformBuffer,
		vk::DescriptorType::eStorageBuffer,
		vk::DescriptorType::eUniformBufferDynamic,
		vk::DescriptorType::eStorageBufferDynamic,
		vk::DescriptorType::eInputAttachment,
	};
	std::vector<vk::DescriptorPoolSize> poolSizes;
	poolSizes.resize(poolTypes.size());
	for (ui32 i = 0; i < poolTypes.size(); ++i)
	{
		poolSizes[i] = vk::DescriptorPoolSize().setType(poolTypes[i]).setDescriptorCount(poolSize);
	}
	auto info = vk::DescriptorPoolCreateInfo()
		.setFlags(vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet)
		.setMaxSets(poolSize)
		.setPoolSizeCount((ui32)poolSizes.size())
		.setPPoolSizes(poolSizes.data());
	return mLogicalDevice.mDevice->createDescriptorPoolUnique(info);
}

void GuiContext::submitFonts()
{
	// since the command pools are re-programmed every frame, it doesn't matter which frame we use
	auto& frame = this->mDynamicFrames[0];
	frame.submitOneOff(
		&mQueues[graphics::QueueFamily::eGraphics],
		[&](vk::UniqueCommandBuffer &buffer)
		{
			ImGui_ImplVulkan_CreateFontsTexture((VkCommandBuffer)buffer.get());
		}
	);
	ImGui_ImplVulkan_DestroyFontUploadObjects();
}

void GuiContext::destroy(graphics::VulkanInstance const *pInstance)
{
	ImGui_ImplVulkan_Shutdown();
	ImGui_ImplSDL2_Shutdown();
	ImGui::DestroyContext();

	mDynamicFrames.clear(); // auto releases internal unique handles on deconstruct of elements
	mRenderPass.destroy();
	mImageViews.clear();
	mSwapChain.destroy();

	mDescriptorPool.reset();
	mQueues.clear(); // queues don't need direct reset so long as they are gone before the logical device
	mLogicalDevice.invalidate();
	mPhysicalDevice.invalidate();

	mSurface.releaseWindowHandle();
	mSurface.destroy(pInstance);
}

void GuiContext::processInput(void *evt)
{
	ImGui_ImplSDL2_ProcessEvent(reinterpret_cast<SDL_Event*>(evt));
}

void GuiContext::render()
{
	this->startFrame();
	this->makeGui();
	this->endFrame();
}

void GuiContext::startFrame()
{
	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplSDL2_NewFrame(reinterpret_cast<SDL_Window*>(mSurface.getWindowHandle()));
	ImGui::NewFrame();
}

void GuiContext::endFrame()
{
	ImGui::Render();

	auto& frame = this->mDynamicFrames[this->mIdxCurrentFrame];
	frame.waitUntilNotInFlight();

	auto idxImage = frame.acquireNextImage(&mSwapChain);
	// vulkan implements the bool operator for validity checks
	if (this->mImagesInFlight[idxImage])
	{
		mLogicalDevice.mDevice->waitForFences(mImagesInFlight[idxImage], true, UINT64_MAX);
	}
	mImagesInFlight[idxImage] = frame.mFence_FrameInFlight.get();
	frame.markNotInFlight();

	this->renderFrame(frame);
	frame.present(&mSwapChain, idxImage, &mQueues[graphics::QueueFamily::eGraphics]);
	
	this->mIdxCurrentFrame = (this->mIdxCurrentFrame + 1) % this->mDynamicFrames.size();
}

void GuiContext::renderFrame(graphics::DynamicFrame &frame)
{
	std::array<f32, 4U> clearColor = { 0.45f, 0.55f, 0.60f, 1.00f };
	frame.beginRenderPass(&mSwapChain, vk::ClearValue().setColor(vk::ClearColorValue(clearColor)));
	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), frame.mCommandBuffer.get());
	frame.endRenderPass();
	frame.submitBuffer(&mQueues[graphics::QueueFamily::eGraphics]);
}

void GuiContext::waitUntilIdle()
{
	this->mLogicalDevice.waitUntilIdle();
}

void GuiContext::makeGui()
{
	ImGui::Begin("Hello, world!");
	ImGui::Text("This is some useful text.");
	ImGui::End();
}
