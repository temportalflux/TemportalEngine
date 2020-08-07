#include "graphics/RenderPass.hpp"

#include "graphics/GraphicsDevice.hpp"
#include "graphics/VulkanApi.hpp"

using namespace graphics;

RenderPass& RenderPass::setClearColor(std::optional<math::Vector4> const color)
{
	this->mClearColor = color;
	return *this;
}

RenderPass& RenderPass::setClearDepthStencil(std::optional<std::pair<f32, ui32>> const depthStencil)
{
	this->mClearDepthStencil = depthStencil;
	return *this;
}

RenderPass& RenderPass::setRenderArea(graphics::Area const area)
{
	this->mRenderArea = area;
	return *this;
}

RenderPass& RenderPass::setImageFormatType(graphics::ImageFormatReferenceType::Enum type, ui32 vkImageFormat)
{
	this->mImageFormatsByType.insert(std::make_pair(type, vkImageFormat));
	return *this;
}

ui32 RenderPass::getImageFormatFor(graphics::ImageFormatReferenceType::Enum type) const
{
	return this->mImageFormatsByType.find(type)->second;
}

RenderPass& RenderPass::addPhase(graphics::RPPhase const &phase)
{
	this->mPhases.push_back(phase);
	return *this;
}

RenderPass& RenderPass::addDependency(graphics::RPDependency const &dependency)
{
	this->mDependencies.push_back(
		vk::SubpassDependency()
		.setSrcSubpass(dependency.dependee.phaseIndex ? (ui32)*dependency.dependee.phaseIndex : VK_SUBPASS_EXTERNAL)
		.setSrcStageMask((vk::PipelineStageFlagBits)dependency.dependee.stageMask.data())
		.setSrcAccessMask((vk::AccessFlagBits)dependency.dependee.accessMask.data())
		.setDstSubpass(dependency.depender.phaseIndex ? (ui32)*dependency.depender.phaseIndex : VK_SUBPASS_EXTERNAL)
		.setDstStageMask((vk::PipelineStageFlagBits)dependency.depender.stageMask.data())
		.setSrcAccessMask((vk::AccessFlagBits)dependency.depender.accessMask.data())
	);
	return *this;
}

bool RenderPass::isValid() const
{
	return (bool)this->mInternal;
}

void RenderPass::addPhaseAsSubpass(graphics::RPPhase const &phase)
{
	auto pushAttachment = [&](graphics::RPPhase::Attachment const &attachment, vk::ImageLayout finalLayout) -> uIndex {
		uIndex idxAttachment = this->mAttachments.size();
		this->mAttachments.push_back(
			vk::AttachmentDescription()
			.setFormat((vk::Format)this->getImageFormatFor(attachment.formatType))
			.setSamples((vk::SampleCountFlagBits)attachment.samples)
			.setLoadOp((vk::AttachmentLoadOp)attachment.generalLoadOp).setStoreOp((vk::AttachmentStoreOp)attachment.generalStoreOp)
			.setStencilLoadOp((vk::AttachmentLoadOp)attachment.stencilLoadOp).setStencilStoreOp((vk::AttachmentStoreOp)attachment.stencilStoreOp)
			.setInitialLayout(vk::ImageLayout::eUndefined)
			.setFinalLayout(finalLayout)
		);
		return idxAttachment;
	};

	this->mSubpassAttachmentRefs.push_back(SubpassAttachments());
	SubpassAttachments& subpassAttachments = this->mSubpassAttachmentRefs[this->mSubpassAttachmentRefs.size() - 1];

	for (auto attachment : phase.colorAttachments)
	{
		auto idxAttachment = pushAttachment(attachment, vk::ImageLayout::ePresentSrcKHR);
		subpassAttachments.color.push_back(
			vk::AttachmentReference()
			.setAttachment((ui32)idxAttachment)
			.setLayout(vk::ImageLayout::eColorAttachmentOptimal)
		);
	}
	if (phase.depthAttachment)
	{
		auto idxAttachment = pushAttachment(*phase.depthAttachment, vk::ImageLayout::eDepthStencilAttachmentOptimal);
		subpassAttachments.depth = vk::AttachmentReference()
			.setAttachment((ui32)idxAttachment)
			.setLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);
	}

	auto subpassDesc = vk::SubpassDescription()
		.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
		.setColorAttachmentCount((ui32)subpassAttachments.color.size())
		.setPColorAttachments(subpassAttachments.color.data());
	if (subpassAttachments.depth)
	{
		subpassDesc.setPDepthStencilAttachment(&subpassAttachments.depth.value());
	}
	this->mSubpasses.push_back(std::move(subpassDesc));
}

void RenderPass::create()
{
	assert(!isValid());

	for (auto const &phase : this->mPhases)
	{
		this->addPhaseAsSubpass(phase);
	}

	auto info = vk::RenderPassCreateInfo()
		.setAttachmentCount((ui32)this->mAttachments.size())
		.setPAttachments(this->mAttachments.data())
		.setSubpassCount((ui32)this->mSubpasses.size())
		.setPSubpasses(this->mSubpasses.data())
		.setDependencyCount((ui32)this->mDependencies.size())
		.setPDependencies(this->mDependencies.data());

	this->mInternal = this->device()->createRenderPass(info);
}

void* RenderPass::get()
{
	return &this->mInternal.get();
}

void RenderPass::invalidate()
{
	this->mInternal.reset();
	this->mAttachments.clear();
	this->mSubpassAttachmentRefs.clear();
	this->mSubpasses.clear();
	this->mImageFormatsByType.clear();
}

void RenderPass::resetConfiguration()
{
	this->mPhases.clear();
	this->mDependencies.clear();
}
