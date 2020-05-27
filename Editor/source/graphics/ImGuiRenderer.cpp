#include "graphics/ImGuiRenderer.hpp"

#include "graphics/VulkanInstance.hpp"
#include "graphics/PhysicalDevice.hpp"
#include "graphics/LogicalDevice.hpp"
#include "graphics/QueueFamily.hpp"
#include "graphics/RenderPass.hpp"

#include <imgui.h>
#include <imgui_internal.h>
#include <examples/imgui_impl_sdl.h>
#include <examples/imgui_impl_vulkan.h>

using namespace graphics;

ImGuiRenderer::ImGuiRenderer(VulkanInstance *pInstance, Surface &surface) : VulkanRenderer(pInstance, surface)
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

	ImGui::StyleColorsDark();

	ImGuiStyle& style = ImGui::GetStyle();
	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		style.WindowRounding = 0.0f;
		style.Colors[ImGuiCol_WindowBg].w = 1.0f;
	}
}

void ImGuiRenderer::initializeDevices()
{
	VulkanRenderer::initializeDevices();
	this->mDescriptorPool = this->createDescriptorPool();
}

void ImGuiRenderer::invalidate()
{
	ImGui_ImplVulkan_Shutdown();
	ImGui_ImplSDL2_Shutdown();
	ImGui::DestroyContext();

	this->mDescriptorPool.reset();

	VulkanRenderer::invalidate();
}

vk::UniqueDescriptorPool ImGuiRenderer::createDescriptorPool()
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
	return this->mLogicalDevice.mDevice->createDescriptorPoolUnique(info);
}

void ImGuiRenderer::createFrames(uSize viewCount)
{
	auto queueFamilyGroup = this->mPhysicalDevice.queryQueueFamilyGroup();
	this->mGuiFrames.resize(viewCount);
	for (uSize i = 0; i < viewCount; ++i)
	{
		this->mGuiFrames[i]
			.setRenderPass(&this->mRenderPass)
			.setView(&this->mImageViews[i])
			.setQueueFamilyGroup(&queueFamilyGroup)
			.create(&this->mLogicalDevice);
	}
}

uSize ImGuiRenderer::getNumberOfFrames() const
{
	return this->mGuiFrames.size();
}

graphics::Frame* ImGuiRenderer::getFrameAt(uSize idx)
{
	return &this->mGuiFrames[idx];
}

void ImGuiRenderer::destroyFrames()
{
	this->mGuiFrames.clear();
}

void ImGuiRenderer::finalizeInitialization()
{
	VulkanRenderer::finalizeInitialization();

	ImGui_ImplSDL2_InitForVulkan(reinterpret_cast<SDL_Window*>(this->mSurface.getWindowHandle()));

	{
		ImGui_ImplVulkan_InitInfo info;
		info.Instance = *reinterpret_cast<VkInstance*>(this->mpInstance->get());
		info.PhysicalDevice = *reinterpret_cast<VkPhysicalDevice*>(this->mPhysicalDevice.get());
		info.Device = *reinterpret_cast<VkDevice*>(this->mLogicalDevice.get());
		auto queueFamilyGroup = this->mPhysicalDevice.queryQueueFamilyGroup();
		info.QueueFamily = queueFamilyGroup.getQueueIndex(graphics::QueueFamily::eGraphics).value();
		info.Queue = (VkQueue)this->mQueues[graphics::QueueFamily::eGraphics];
		info.PipelineCache = VK_NULL_HANDLE;
		info.DescriptorPool = (VkDescriptorPool)mDescriptorPool.get();
		info.Allocator = nullptr;
		info.MSAASamples = (VkSampleCountFlagBits)vk::SampleCountFlagBits::e1;
		info.MinImageCount = (ui32)this->mImageViews.size();
		info.ImageCount = (ui32)this->mImageViews.size();
		info.CheckVkResultFn = nullptr;
		ImGui_ImplVulkan_Init(&info, *reinterpret_cast<VkRenderPass*>(this->mRenderPass.get()));
	}
	
	this->submitFonts();
}

void ImGuiRenderer::submitFonts()
{
	auto createFonts = [&](CommandBuffer &buffer)
	{
		ImGui_ImplVulkan_CreateFontsTexture(*((VkCommandBuffer*)buffer.get()));
	};
	this->mGuiFrames[0].submitOneOff(&this->getQueue(QueueFamily::eGraphics), createFonts);
	ImGui_ImplVulkan_DestroyFontUploadObjects();
}

void ImGuiRenderer::onInputEvent(void* evt)
{
	ImGui_ImplSDL2_ProcessEvent(reinterpret_cast<SDL_Event*>(evt));
}

void ImGuiRenderer::drawFrame()
{
	if (this->mbRenderChainDirty) return;

	this->startGuiFrame();
	this->makeGui();
	this->endGuiFrame();

	VulkanRenderer::drawFrame();

	auto& io = ImGui::GetIO();
	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault();
	}
}

void ImGuiRenderer::startGuiFrame()
{
	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplSDL2_NewFrame(reinterpret_cast<SDL_Window*>(this->mSurface.getWindowHandle()));
	ImGui::NewFrame();
}

void ImGuiRenderer::makeGui()
{
	ImGui::Begin("Hello, world!");
	ImGui::Text("This is some useful text.");
	ImGui::Button("Button");
	ImGui::End();

	ImGui::Begin("Window 2");
	ImGui::Text("This is some meaningless text.");
	ImGui::End();
}

void ImGuiRenderer::endGuiFrame()
{
	ImGui::Render();
}

void ImGuiRenderer::render()
{
	auto* currentFrame = reinterpret_cast<ImGuiFrame*>(this->getFrameAt(this->mIdxCurrentFrame));
	
	std::array<f32, 4U> clearColor = { 0.45f, 0.55f, 0.60f, 1.00f };
	auto cmd = currentFrame->beginRenderPass(&mSwapChain, clearColor);
	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), currentFrame->getRawBuffer());
	currentFrame->endRenderPass(cmd);
	currentFrame->submitBuffers(&this->mQueues[QueueFamily::eGraphics], {});
}
