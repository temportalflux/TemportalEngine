#include "graphics/RenderPass.hpp"

#include "graphics/GraphicsDevice.hpp"
#include "graphics/VulkanApi.hpp"

using namespace graphics;

RenderPassAttachment& RenderPassAttachment::setFormat(ui32 const value)
{
	this->mFormatValue = value;
	return *this;
}

RenderPassAttachment& RenderPassAttachment::setSamples(vk::SampleCountFlagBits const flags)
{
	this->mSampleFlags = flags;
	return *this;
}

RenderPassAttachment& RenderPassAttachment::setGeneralOperations(vk::AttachmentLoadOp load, vk::AttachmentStoreOp store)
{
	this->mGeneralLoad = load;
	this->mGeneralStore = store;
	return *this;
}

RenderPassAttachment& RenderPassAttachment::setStencilOperations(vk::AttachmentLoadOp load, vk::AttachmentStoreOp store)
{
	this->mStencilLoad = load;
	this->mStencilStore = store;
	return *this;
}

RenderPassAttachment& RenderPassAttachment::setLayouts(vk::ImageLayout initialLayout, vk::ImageLayout finalLayout)
{
	this->mLayoutInitial = initialLayout;
	this->mLayoutFinal = finalLayout;
	return *this;
}

vk::AttachmentDescription RenderPassAttachment::description() const
{
	return vk::AttachmentDescription()
		.setFormat((vk::Format)this->mFormatValue)
		.setSamples(this->mSampleFlags)
		.setLoadOp(this->mGeneralLoad).setStoreOp(this->mGeneralStore)
		.setStencilLoadOp(this->mStencilLoad).setStencilStoreOp(this->mStencilStore)
		.setInitialLayout(this->mLayoutInitial)
		.setFinalLayout(this->mLayoutFinal);
}

RenderPassPhase& RenderPassPhase::addColorAttachment(RenderPassAttachment const &attachment)
{
	assert(attachment.mIdxInRenderPass);
	this->mColorAttachmentReferences.push_back(
		vk::AttachmentReference()
		// the index of the attachment in the `attachments` list in RenderPass
		.setAttachment((ui32)*attachment.mIdxInRenderPass)
		.setLayout(vk::ImageLayout::eColorAttachmentOptimal)
	);
	return *this;
}

RenderPassPhase& RenderPassPhase::setDepthAttachment(RenderPassAttachment const &attachment)
{
	assert(attachment.mIdxInRenderPass);
	this->mDepthStencilAttachmentReference = vk::AttachmentReference()
		// the index of the attachment in the `attachments` list in RenderPass
		.setAttachment((ui32)*attachment.mIdxInRenderPass)
		.setLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);
	return *this;
}

vk::SubpassDescription RenderPassPhase::description() const
{
	auto& desc = vk::SubpassDescription().setPipelineBindPoint(vk::PipelineBindPoint::eGraphics);
	desc
		.setColorAttachmentCount((ui32)this->mColorAttachmentReferences.size())
		.setPColorAttachments(this->mColorAttachmentReferences.data());
	if (this->mDepthStencilAttachmentReference)
	{
		desc.setPDepthStencilAttachment(&this->mDepthStencilAttachmentReference.value());
	}
	return desc;
}

RenderPassAttachment& RenderPass::addAttachment(RenderPassAttachment &attachment)
{
	attachment.mIdxInRenderPass = this->mAttachments.size();
	this->mAttachments.push_back(attachment);
	return attachment;
}

RenderPassPhase& RenderPass::addPhase(RenderPassPhase &phase)
{
	phase.mIdxInRenderPass = this->mPhases.size();
	this->mPhases.push_back(phase);
	return phase;
}

RenderPass& RenderPass::addDependency(DependencyItem const dependee, DependencyItem const depender)
{
	assert(!dependee.phase.has_value() || dependee.phase.value().mIdxInRenderPass);
	assert(!depender.phase.has_value() || depender.phase.value().mIdxInRenderPass);
	assert(dependee.phase || depender.phase);
	this->mDependencies.push_back(
		vk::SubpassDependency()
		.setSrcSubpass(dependee.phase ? (ui32)*dependee.phase->mIdxInRenderPass : VK_SUBPASS_EXTERNAL)
		.setSrcStageMask(dependee.stageFlags).setSrcAccessMask(dependee.accessFlags)
		.setDstSubpass(depender.phase ? (ui32)*depender.phase->mIdxInRenderPass : VK_SUBPASS_EXTERNAL)
		.setDstStageMask(depender.stageFlags).setDstAccessMask(depender.accessFlags)
	);
	return *this;
}

RenderPass& RenderPass::create(std::shared_ptr<GraphicsDevice> device)
{
	assert(!isValid());

	auto attachments = std::vector<vk::AttachmentDescription>(this->mAttachments.size());
	std::transform(
		this->mAttachments.begin(), this->mAttachments.end(),
		attachments.begin(), [](auto& attachment) { return attachment.description(); }
	);

	auto subpassDescs = std::vector<vk::SubpassDescription>(this->mPhases.size());
	std::transform(
		this->mPhases.begin(), this->mPhases.end(),
		subpassDescs.begin(), [](auto& phase) { return phase.description(); }
	);

	auto info = vk::RenderPassCreateInfo()
		.setAttachmentCount((ui32)attachments.size())
		.setPAttachments(attachments.data())
		.setSubpassCount((ui32)subpassDescs.size())
		.setPSubpasses(subpassDescs.data())
		.setDependencyCount((ui32)this->mDependencies.size())
		.setPDependencies(this->mDependencies.data());

	this->mRenderPass = device->createRenderPass(info);
	return *this;
}

bool RenderPass::isValid() const
{
	return (bool)this->mRenderPass;
}

void* RenderPass::get()
{
	return &this->mRenderPass.get();
}

void RenderPass::destroy()
{
	this->mRenderPass.reset();
}

void RenderPass::reset()
{
	this->destroy();
	this->mAttachments.clear();
	this->mPhases.clear();
	this->mDependencies.clear();
}
