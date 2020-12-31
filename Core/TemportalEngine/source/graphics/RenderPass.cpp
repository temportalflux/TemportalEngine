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

RenderPass& RenderPass::setImageFormatType(graphics::EImageFormatCategory type, ui32 vkImageFormat)
{
	this->mImageFormatsByType.insert(std::make_pair(type, vkImageFormat));
	return *this;
}

ui32 RenderPass::getImageFormatFor(graphics::EImageFormatCategory type) const
{
	return this->mImageFormatsByType.find(type)->second;
}

RenderPass& RenderPass::setAttachments(std::vector<graphics::RenderPassAttachment> const& attachments)
{
	this->mAttachments = attachments;
	return *this;
}

RenderPass& RenderPass::setPhases(std::vector<graphics::RenderPassPhase> const& phases)
{
	this->mPhases = phases;
	return *this;
}

RenderPass& RenderPass::setPhaseDependencies(std::vector<graphics::RenderPassDependency> const& dependencies)
{
	this->mDependencies = dependencies;
	return *this;
}

bool RenderPass::isValid() const
{
	return (bool)this->mInternal;
}

void RenderPass::create()
{
	assert(!isValid());

	auto attachments = std::vector<vk::AttachmentDescription>();
	for (auto const& attachment : this->mAttachments)
	{
		attachments.push_back(
			vk::AttachmentDescription()
			.setFormat((vk::Format)this->getImageFormatFor(attachment.formatType))
			.setSamples(attachment.samples.as<vk::SampleCountFlagBits>())
			.setLoadOp(attachment.generalLoadOp.as<vk::AttachmentLoadOp>())
			.setStoreOp(attachment.generalStoreOp.as<vk::AttachmentStoreOp>())
			.setStencilLoadOp(attachment.stencilLoadOp.as<vk::AttachmentLoadOp>())
			.setStencilStoreOp(attachment.stencilStoreOp.as<vk::AttachmentStoreOp>())
			.setInitialLayout(attachment.initialLayout.as<vk::ImageLayout>())
			.setFinalLayout(attachment.finalLayout.as<vk::ImageLayout>())
		);
	}

	auto subpasses = std::vector<vk::SubpassDescription>();
	struct SubpassAttachmentReferenceSet
	{
		std::vector<vk::AttachmentReference> colors;
		vk::AttachmentReference depth;
	};
	auto attachmentReferences = std::vector<SubpassAttachmentReferenceSet>();
	for (auto const& phase : this->mPhases)
	{
		auto iter = attachmentReferences.insert(attachmentReferences.end(), SubpassAttachmentReferenceSet{});
		SubpassAttachmentReferenceSet& attachRefSet = *iter;
		auto desc = vk::SubpassDescription().setPipelineBindPoint(vk::PipelineBindPoint::eGraphics);
		
		for (auto const& attachment : phase.colorAttachments)
		{
			if (!attachment) continue;
			attachRefSet.colors.push_back(
				vk::AttachmentReference()
				.setAttachment((ui32)attachment->attachment)
				.setLayout(attachment->layout.as<vk::ImageLayout>())
			);
		}
		desc
			.setColorAttachmentCount((ui32)attachRefSet.colors.size())
			.setPColorAttachments(attachRefSet.colors.data());
		
		if (auto attachment = phase.depthAttachment)
		{
			attachRefSet.depth = vk::AttachmentReference()
				.setAttachment((ui32)attachment->attachment)
				.setLayout(attachment->layout.as<vk::ImageLayout>());
			desc.setPDepthStencilAttachment(&attachRefSet.depth);
		}

		subpasses.push_back(std::move(desc));
	}
	
	auto dependencies = std::vector<vk::SubpassDependency>();
	for (auto const& dependency : this->mDependencies)
	{
		dependencies.push_back(
			vk::SubpassDependency()
			.setSrcSubpass(dependency.prev.phase ? (ui32)*dependency.prev.phase : VK_SUBPASS_EXTERNAL)
			.setSrcStageMask((vk::PipelineStageFlagBits)dependency.prev.stageMask.data())
			.setSrcAccessMask((vk::AccessFlagBits)dependency.prev.accessMask.data())
			.setDstSubpass(dependency.next.phase ? (ui32)*dependency.next.phase : VK_SUBPASS_EXTERNAL)
			.setDstStageMask((vk::PipelineStageFlagBits)dependency.next.stageMask.data())
			.setSrcAccessMask((vk::AccessFlagBits)dependency.next.accessMask.data())
		);
	}

	auto info = vk::RenderPassCreateInfo()
		.setAttachmentCount((ui32)attachments.size())
		.setPAttachments(attachments.data())
		.setSubpassCount((ui32)subpasses.size())
		.setPSubpasses(subpasses.data())
		.setDependencyCount((ui32)dependencies.size())
		.setPDependencies(dependencies.data());

	this->mInternal = this->device()->createRenderPass(info);
}

void* RenderPass::get()
{
	return &this->mInternal.get();
}

void RenderPass::invalidate()
{
	this->mInternal.reset();
	this->mImageFormatsByType.clear();
}

void RenderPass::resetConfiguration()
{
	this->mAttachments.clear();
	this->mPhases.clear();
	this->mDependencies.clear();
}
