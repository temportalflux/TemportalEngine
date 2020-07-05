#include "graphics/ImGuiRenderer.hpp"

#include "graphics/DescriptorPool.hpp"
#include "graphics/VulkanInstance.hpp"
#include "graphics/PhysicalDevice.hpp"
#include "graphics/LogicalDevice.hpp"
#include "graphics/RenderPass.hpp"
#include "graphics/VulkanApi.hpp"
#include "gui/IGui.hpp"

#include <imgui.h>
#include <imgui_internal.h>
#include <examples/imgui_impl_sdl.h>
#include <examples/imgui_impl_vulkan.h>

using namespace graphics;

ImGuiRenderer::ImGuiRenderer() : VulkanRenderer()
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
	this->createDescriptorPoolImgui();
}

void ImGuiRenderer::invalidate()
{
	for (auto&[id, gui] : this->mGuis)
	{
		gui->onRemovedFromRenderer(this);
	}
	this->mGuis.clear();

	ImGui_ImplVulkan_Shutdown();
	ImGui_ImplSDL2_Shutdown();
	ImGui::DestroyContext();

	this->mDescriptorPool.invalidate();

	this->destroyRenderChain();
	VulkanRenderer::invalidate();
}

void ImGuiRenderer::createRenderChain()
{
	this->createSwapChain();
	this->createFrameImageViews();
	this->createRenderPass();
	this->createFrames(this->mFrameImageViews.size());
}

void ImGuiRenderer::destroyRenderChain()
{
	this->destroyFrames();
	this->destroyRenderPass();
	this->destroyFrameImageViews();
	this->destroySwapChain();
}

void ImGuiRenderer::createRenderPass()
{
	auto& colorAttachment = this->mRenderPass.addAttachment(
		RenderPassAttachment()
		.setFormat(this->mSwapChain.getFormat())
		.setSamples(vk::SampleCountFlagBits::e1)
		.setGeneralOperations(vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore)
		.setStencilOperations(vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare)
		.setLayouts(vk::ImageLayout::eUndefined, vk::ImageLayout::ePresentSrcKHR)
	);

	auto& onlyPhase = this->mRenderPass.addPhase(
		RenderPassPhase()
		.addColorAttachment(colorAttachment)
	);

	this->mRenderPass.addDependency(
		{ std::nullopt, vk::PipelineStageFlagBits::eColorAttachmentOutput },
		{ onlyPhase, vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::AccessFlagBits::eColorAttachmentWrite }
	);

	this->mRenderPass.create(this->mpGraphicsDevice);
}

RenderPass* ImGuiRenderer::getRenderPass()
{
	return &this->mRenderPass;
}

void ImGuiRenderer::destroyRenderPass()
{
	this->mRenderPass.destroy();
}

void ImGuiRenderer::createDescriptorPoolImgui()
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
	std::unordered_map<vk::DescriptorType, ui32> poolSizes;
	for (ui32 i = 0; i < poolTypes.size(); ++i)
	{
		poolSizes.insert(std::make_pair(poolTypes[i], poolSize));
	}

	ui32 frameCount = 3;
	this->mDescriptorPool
		.setFlags(vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet)
		.setPoolSize(frameCount, poolSizes)
		.create(this->mpGraphicsDevice, frameCount);
}

void ImGuiRenderer::createFrames(uSize viewCount)
{
	auto queueFamilyGroup = this->mpGraphicsDevice->queryQueueFamilyGroup();
	this->mGuiFrames.resize(viewCount);
	for (uSize i = 0; i < viewCount; ++i)
	{
		this->mGuiFrames[i]
			.setRenderPass(&this->mRenderPass)
			.setResolution(this->mSwapChain.getResolution())
			.setView(&this->mFrameImageViews[i])
			.setQueueFamilyGroup(&queueFamilyGroup)
			.create(this->mpGraphicsDevice);
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
		info.Instance = extract<VkInstance>(this->mpInstance.get());
		info.PhysicalDevice = extract<VkPhysicalDevice>(&this->mpGraphicsDevice->physical());
		info.Device = extract<VkDevice>(&this->mpGraphicsDevice->logical());
		auto queueFamilyGroup = this->mpGraphicsDevice->queryQueueFamilyGroup();
		info.QueueFamily = queueFamilyGroup.getQueueIndex(graphics::QueueFamily::Enum::eGraphics).value();
		info.Queue = (VkQueue)this->getQueue(graphics::QueueFamily::Enum::eGraphics);
		info.PipelineCache = VK_NULL_HANDLE;
		info.DescriptorPool = extract<VkDescriptorPool>(&this->mDescriptorPool);
		info.Allocator = nullptr;
		info.MSAASamples = (VkSampleCountFlagBits)vk::SampleCountFlagBits::e1;
		info.MinImageCount = (ui32)this->mFrameImageViews.size();
		info.ImageCount = (ui32)this->mFrameImageViews.size();
		info.CheckVkResultFn = nullptr;
		ImGui_ImplVulkan_Init(&info, extract<VkRenderPass>(&this->mRenderPass));
	}
	
	this->submitFonts();
}

void ImGuiRenderer::submitFonts()
{
	auto createFonts = [&](CommandBuffer &buffer)
	{
		ImGui_ImplVulkan_CreateFontsTexture(graphics::extract<VkCommandBuffer>(&buffer));
	};
	this->mGuiFrames[0].submitOneOff(&this->getQueue(QueueFamily::Enum::eGraphics), createFonts);
	ImGui_ImplVulkan_DestroyFontUploadObjects();
}

void ImGuiRenderer::addGui(std::string id, std::shared_ptr<gui::IGui> gui)
{
	this->mGuis.insert(std::make_pair(id, gui));
	gui->onAddedToRenderer(this);
}

std::shared_ptr<gui::IGui> ImGuiRenderer::removeGui(std::string id)
{
	auto guiIter = this->mGuis.find(id);
	assert(guiIter != this->mGuis.end());
	// add gui ids to remove when the current frame is done being iterated over
	this->mGuisToRemove.push_back(id);
	return guiIter->second;
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

	for (auto& id : this->mGuisToRemove)
	{
		auto guiIter = this->mGuis.find(id);
		auto gui = guiIter->second;
		this->mGuis.erase(guiIter);
		gui->onRemovedFromRenderer(this);
	}
	this->mGuisToRemove.clear();
}

void ImGuiRenderer::startGuiFrame()
{
	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplSDL2_NewFrame(reinterpret_cast<SDL_Window*>(this->mSurface.getWindowHandle()));
	ImGui::NewFrame();
}

void ImGuiRenderer::makeGui()
{
	for (auto& [id, pGui]: this->mGuis)
	{
		assert(pGui);
		pGui->makeGui();
	}
}

void ImGuiRenderer::endGuiFrame()
{
	ImGui::Render();
}

void ImGuiRenderer::render(graphics::Frame* frame, ui32 idxCurrentImage)
{
	auto* currentFrame = reinterpret_cast<ImGuiFrame*>(frame);
	
	std::array<f32, 4U> clearColor = { 0.0f, 0.0f, 0.0f, 1.00f };
	auto cmd = currentFrame->beginRenderPass(&mSwapChain, clearColor);
	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), graphics::extract<VkCommandBuffer>(&currentFrame->cmdBuffer()));
	currentFrame->endRenderPass(cmd);
	currentFrame->submitBuffers(&this->getQueue(QueueFamily::Enum::eGraphics), {});
}
