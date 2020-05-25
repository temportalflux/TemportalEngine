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

bool RenderPass::isValid() const
{
	return (bool)this->mRenderPass;
}

RenderPass& RenderPass::create(LogicalDevice const *pDevice)
{
	assert(!isValid());

	mpDevice = pDevice;

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

	auto subpassDesc = vk::SubpassDescription()
		.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
		.setColorAttachmentCount(1)
		.setPColorAttachments(&refColorAttachment);

	auto subpassDependency = vk::SubpassDependency()
		.setSrcSubpass(VK_SUBPASS_EXTERNAL)
		.setDstSubpass(0)
		.setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput).setSrcAccessMask({})
		.setDstStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput).setDstAccessMask(vk::AccessFlagBits::eColorAttachmentWrite);

	auto info = vk::RenderPassCreateInfo()
		.setAttachmentCount(1)
		.setPAttachments(&colorAttachment)
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

std::vector<FrameBuffer> RenderPass::createFrameBuffers(std::vector<vk::UniqueImageView> const &views) const
{
	auto viewCount = views.size();
	auto buffers = std::vector<FrameBuffer>(viewCount);
	for (uSize i = 0; i < viewCount; ++i)
	{
		buffers[i].setRenderPass(this).setView(views[i].get()).create(mpDevice);
	}
	return buffers;
}
