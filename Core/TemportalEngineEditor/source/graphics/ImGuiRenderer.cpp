#include "graphics/ImGuiRenderer.hpp"

#include "Engine.hpp"
#include "graphics/DescriptorPool.hpp"
#include "graphics/ImGuiTexture.hpp"
#include "graphics/LogicalDevice.hpp"
#include "graphics/PhysicalDevice.hpp"
#include "graphics/RenderPass.hpp"
#include "graphics/VulkanApi.hpp"
#include "graphics/VulkanInstance.hpp"
#include "gui/IGui.hpp"

#include <imgui.h>
#include <imgui_internal.h>
#include <backends/imgui_impl_sdl.h>
#include <backends/imgui_impl_vulkan.h>

using namespace graphics;

logging::Logger IMGUI_LOG = DeclareLog("ImGui");

void ImGuiRenderer::renderImGui(const ImDrawList* parent_list, const ImDrawCmd* cmd)
{
	reinterpret_cast<ImGuiRenderer*>(cmd->UserCallbackData)->renderDrawData(parent_list, cmd);
}

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
	this->initializeTransientCommandPool();
}

void ImGuiRenderer::invalidate()
{
	this->mGuis.clear();
	this->mUnusedTextures.clear();

	ImGui_ImplVulkan_Shutdown();
	ImGui_ImplSDL2_Shutdown();
	ImGui::DestroyContext();

	this->mDescriptorPool.invalidate();
	this->mCommandPoolTransient.invalidate();

	this->destroyRenderChain();
	VulkanRenderer::invalidate();
}

CommandPool& ImGuiRenderer::getTransientPool()
{
	return this->mCommandPoolTransient;
}

void ImGuiRenderer::initializeTransientCommandPool()
{
	auto device = this->getDevice();
	this->mCommandPoolTransient.setDevice(device);
	this->mCommandPoolTransient
		.setFlags(vk::CommandPoolCreateFlagBits::eTransient)
		.setQueueFamily(
			graphics::EQueueFamily::eGraphics,
			device->queryQueueFamilyGroup()
		)
		.create();
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
	this->mRenderPass.setDevice(this->mpGraphicsDevice);
	this->mRenderPass
		.setClearColor(math::Vector4({ 0.0f, 0.0f, 0.0f, 1.0f }))
		.setRenderArea({ { 0, 0 }, { 1, 1 } })
		.setImageFormatType(EImageFormatCategory::Viewport, this->mSwapChain.getFormat());
	
	this->mRenderPass.setAttachments({
			{
				graphics::EImageFormatCategory::Viewport,
				graphics::ESampleCount::e1,
				graphics::EAttachmentLoadOp::eClear, graphics::EAttachmentStoreOp::eStore,
				graphics::EAttachmentLoadOp::eDontCare, graphics::EAttachmentStoreOp::eDontCare,
				graphics::EImageLayout::eUndefined, graphics::EImageLayout::ePresentSrc
			}
	});
	this->mRenderPass.setPhases({
		// phase
		{
			// color attachments
			{
				graphics::RenderPassAttachmentReference { 0, graphics::EImageLayout::eColorAttachmentOptimal }
			},
			// depth attachment
			std::nullopt
		}
	});
	this->mRenderPass.setPhaseDependencies({
		{
			// prev
			{
				std::nullopt,
				graphics::PipelineStageFlags::eColorAttachmentOutput,
				{}
			},
			// next
			{
				0,
				graphics::PipelineStageFlags::eColorAttachmentOutput,
				graphics::AccessFlags::eColorAttachmentWrite
			}
		}
	});

	this->mRenderPass.create();
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
	this->mDescriptorPool.setDevice(this->mpGraphicsDevice);
	this->mDescriptorPool
		.setFlags(vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet)
		.setPoolSize(frameCount * poolSize, poolSizes)
		.setAllocationMultiplier(frameCount)
		.create();
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
		info.QueueFamily = queueFamilyGroup.getQueueIndex(graphics::EQueueFamily::eGraphics).value();
		info.Queue = (VkQueue)this->getQueue(graphics::EQueueFamily::eGraphics);
		info.PipelineCache = VK_NULL_HANDLE;
		info.DescriptorPool = extract<VkDescriptorPool>(&this->mDescriptorPool);
		info.Allocator = nullptr;
		info.MSAASamples = (VkSampleCountFlagBits)vk::SampleCountFlagBits::e1;
		info.MinImageCount = (ui32)this->mFrameImageViews.size();
		info.ImageCount = (ui32)this->mFrameImageViews.size();
		info.Subpass = 0;
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
	this->mGuiFrames[0].submitOneOff(&this->getQueue(EQueueFamily::eGraphics), createFonts);
	ImGui_ImplVulkan_DestroyFontUploadObjects();
}

void ImGuiRenderer::addGui(std::shared_ptr<gui::IGui> gui)
{
	this->mGuisToOpen.push_back(gui);
}

void ImGuiRenderer::removeGui(std::weak_ptr<gui::IGui> const &gui)
{
	this->mGuisToClose.push_back(gui);
}

std::shared_ptr<ImGuiTexture> ImGuiRenderer::reserveTexture()
{
	auto unusedCount = this->mUnusedTextures.size();
	if (unusedCount > 0)
	{
		auto texture = this->mUnusedTextures[unusedCount - 1];
		this->mUnusedTextures.pop_back();
		return texture;
	}
	auto textureId = ImGui_ImplVulkan_CreateTexture();
	if (!textureId)
	{
		IMGUI_LOG.log(LOG_ERR, "Failed to create texture id");
		return nullptr;
	}
	return std::make_shared<ImGuiTexture>(this->shared_from_this(), textureId);
}

void ImGuiRenderer::releaseTextureToPool(std::shared_ptr<ImGuiTexture> texture)
{
	this->mUnusedTextures.push_back(texture);
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

	this->mGuis.erase(std::remove_if(
		this->mGuis.begin(), this->mGuis.end(),
		[&](std::shared_ptr<gui::IGui> const &gui) -> bool
		{
			for (auto& guiToInvalidate : this->mGuisToClose)
			{
				if (guiToInvalidate.lock() == gui) return true;
			}
			return false;
		}
	), this->mGuis.end());
	this->mGuisToClose.clear();
	this->mGuis.insert(this->mGuis.end(), this->mGuisToOpen.begin(), this->mGuisToOpen.end());
	this->mGuisToOpen.clear();
}

void ImGuiRenderer::startGuiFrame()
{
	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplSDL2_NewFrame(reinterpret_cast<SDL_Window*>(this->mSurface.getWindowHandle()));
	ImGui::NewFrame();
}

void ImGuiRenderer::makeGui()
{
	for (auto& pGui: this->mGuis)
	{
		if (pGui)
		{
			pGui->makeGui();
		}
	}
}

void ImGuiRenderer::endGuiFrame()
{
	ImGui::Render();
}

void ImGuiRenderer::render(graphics::Frame* frame, ui32 idxCurrentImage)
{
	this->mpCurrentFrame = reinterpret_cast<ImGuiFrame*>(frame);
	this->mpCurrentBufferOffsets = { 0, 0 };
	
	auto cmdBuffer = graphics::extract<VkCommandBuffer>(&this->mpCurrentFrame->cmdBuffer());

	auto cmd = this->mpCurrentFrame->beginRenderPass(&mSwapChain);
	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmdBuffer);
	this->mpCurrentFrame->endRenderPass(cmd);
	this->mpCurrentFrame->submitBuffers(&this->getQueue(EQueueFamily::eGraphics), {});

	this->mpCurrentFrame = nullptr;
}

void ImGuiRenderer::renderDrawData(const ImDrawList* cmd_list, const ImDrawCmd* pcmd)
{
	auto command_buffer = graphics::extract<VkCommandBuffer>(&this->mpCurrentFrame->cmdBuffer());

	ImDrawData const* draw_data = ImGui::GetDrawData();
	const int fb_width = (int)(draw_data->DisplaySize.x * draw_data->FramebufferScale.x);
	const int fb_height = (int)(draw_data->DisplaySize.y * draw_data->FramebufferScale.y);

	// Will project scissor/clipping rectangles into framebuffer space
	ImVec2 clip_off = draw_data->DisplayPos;         // (0,0) unless using multi-viewports
	ImVec2 clip_scale = draw_data->FramebufferScale; // (1,1) unless using retina display which are often (2,2)

	// Project scissor/clipping rectangles into framebuffer space
	ImVec4 clip_rect;
	clip_rect.x = (pcmd->ClipRect.x - clip_off.x) * clip_scale.x;
	clip_rect.y = (pcmd->ClipRect.y - clip_off.y) * clip_scale.y;
	clip_rect.z = (pcmd->ClipRect.z - clip_off.x) * clip_scale.x;
	clip_rect.w = (pcmd->ClipRect.w - clip_off.y) * clip_scale.y;

	if (clip_rect.x < fb_width && clip_rect.y < fb_height && clip_rect.z >= 0.0f && clip_rect.w >= 0.0f)
	{
		// Negative offsets are illegal for vkCmdSetScissor
		if (clip_rect.x < 0.0f)
			clip_rect.x = 0.0f;
		if (clip_rect.y < 0.0f)
			clip_rect.y = 0.0f;

		// Apply scissor/clipping rectangle
		VkRect2D scissor;
		scissor.offset.x = (int32_t)(clip_rect.x);
		scissor.offset.y = (int32_t)(clip_rect.y);
		scissor.extent.width = (uint32_t)(clip_rect.z - clip_rect.x);
		scissor.extent.height = (uint32_t)(clip_rect.w - clip_rect.y);
		vkCmdSetScissor(command_buffer, 0, 1, &scissor);

		// Draw
		vkCmdDrawIndexed(
			command_buffer, pcmd->ElemCount, 1,
			pcmd->IdxOffset + this->mpCurrentBufferOffsets.y(),
			pcmd->VtxOffset + this->mpCurrentBufferOffsets.x(),
			0
		);
	}

	auto* lastCmdInList = &cmd_list->CmdBuffer[cmd_list->CmdBuffer.Size - 1];
	if (pcmd == lastCmdInList)
	{
		this->mpCurrentBufferOffsets += { cmd_list->VtxBuffer.Size, cmd_list->IdxBuffer.Size };
	}
}
