#include "graphics/RenderPass.hpp"

#include "graphics/LogicalDevice.hpp"
#include "graphics/SwapChain.hpp"

using namespace graphics;

RenderPass::RenderPass()
{
}

RenderPass& RenderPass::initFromSwapChain(SwapChain const *pSwapChain)
{
	mFormat = pSwapChain->mSurfaceFormat.format;
	mResolution = pSwapChain->mResolution; // for usage in FrameBuffer
	return *this;
}

void* RenderPass::get()
{
	return &this->mRenderPass.get();
}

bool RenderPass::isValid() const
{
	return (bool)this->mRenderPass;
}

RenderPass& RenderPass::create(LogicalDevice const *pDevice, std::optional<vk::Format> depthBufferFormat)
{
	assert(!isValid());

	mpDevice = pDevice;
	std::vector<vk::AttachmentDescription> attachments;

	// TODO: All of these can be configured by a static class/structure asset in editor

	auto colorAttachment = vk::AttachmentDescription()
		.setFormat(mFormat)
		.setSamples(vk::SampleCountFlagBits::e1)
		.setLoadOp(vk::AttachmentLoadOp::eClear)
		.setStoreOp(vk::AttachmentStoreOp::eStore)
		.setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
		.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
		.setInitialLayout(vk::ImageLayout::eUndefined)
		.setFinalLayout(vk::ImageLayout::ePresentSrcKHR);

	// Attachs an image to a property in the shader:
	// layout(location = 0) out vec4 outColor
	auto refColorAttachment = vk::AttachmentReference()
		.setLayout(vk::ImageLayout::eColorAttachmentOptimal)
		// index of the attachment in the subpass attachments array
		.setAttachment(0);
	attachments.push_back(colorAttachment);

	auto refDepthAttachment = vk::AttachmentReference();
	if (depthBufferFormat)
	{
		auto depthAttachment = vk::AttachmentDescription()
			.setFormat(*depthBufferFormat)
			.setSamples(vk::SampleCountFlagBits::e1)
			.setLoadOp(vk::AttachmentLoadOp::eClear)
			.setStoreOp(vk::AttachmentStoreOp::eDontCare)
			.setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
			.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
			.setInitialLayout(vk::ImageLayout::eUndefined)
			.setFinalLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);
		refDepthAttachment
			.setLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal)
			// index of the attachment in the subpass attachments array
			.setAttachment(1);
		attachments.push_back(depthAttachment);
	}

	auto subpassDesc = vk::SubpassDescription()
		.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
		.setColorAttachmentCount(1)
		.setPColorAttachments(&refColorAttachment);
	if (depthBufferFormat)
		subpassDesc.setPDepthStencilAttachment(&refDepthAttachment);

	auto subpassDependency = vk::SubpassDependency()
		.setSrcSubpass(VK_SUBPASS_EXTERNAL)
		.setDstSubpass(0)
		.setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput).setSrcAccessMask({})
		.setDstStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput).setDstAccessMask(vk::AccessFlagBits::eColorAttachmentWrite);

	auto info = vk::RenderPassCreateInfo()
		.setAttachmentCount((ui32)attachments.size())
		.setPAttachments(attachments.data())
		.setSubpassCount(1)
		.setPSubpasses(&subpassDesc)
		.setDependencyCount(1)
		.setPDependencies(&subpassDependency);

	mRenderPass = mpDevice->mDevice->createRenderPassUnique(info);
	return *this;
}

void RenderPass::destroy()
{
	this->mRenderPass.reset();
}

vk::RenderPass RenderPass::getRenderPass() const
{
	return mRenderPass.get();
}
