#pragma once

#include "TemportalEnginePCH.hpp"

#include "graphics/FrameBuffer.hpp"
#include "graphics/ImageView.hpp"
#include "math/Vector.hpp"

#include <vector>
#include <vulkan/vulkan.hpp>

NS_GRAPHICS
class GraphicsDevice;

class RenderPassAttachment
{
	friend class RenderPass;
	friend class RenderPassPhase;

public:
	RenderPassAttachment& setFormat(ui32 const value);
	RenderPassAttachment& setSamples(vk::SampleCountFlagBits const flags);
	RenderPassAttachment& setGeneralOperations(vk::AttachmentLoadOp load, vk::AttachmentStoreOp store);
	RenderPassAttachment& setStencilOperations(vk::AttachmentLoadOp load, vk::AttachmentStoreOp store);
	RenderPassAttachment& setLayouts(vk::ImageLayout initialLayout, vk::ImageLayout finalLayout);

private:
	std::optional<uIndex> mIdxInRenderPass;

	ui32 mFormatValue;
	vk::SampleCountFlagBits mSampleFlags;
	vk::AttachmentLoadOp mGeneralLoad, mStencilLoad;
	vk::AttachmentStoreOp mGeneralStore, mStencilStore;
	vk::ImageLayout mLayoutInitial, mLayoutFinal;

	vk::AttachmentDescription description() const;

};

class RenderPassPhase
{
	friend class RenderPass;

public:
	RenderPassPhase() = default;
	RenderPassPhase(RenderPassPhase const &other)
	{
		this->mIdxInRenderPass = other.mIdxInRenderPass;
		this->mColorAttachmentReferences = other.mColorAttachmentReferences;
		this->mDepthStencilAttachmentReference = other.mDepthStencilAttachmentReference;
	}
	RenderPassPhase(RenderPassPhase &&other)
	{
		this->mIdxInRenderPass = std::move(other.mIdxInRenderPass);
		this->mColorAttachmentReferences = std::move(other.mColorAttachmentReferences);
		this->mDepthStencilAttachmentReference = std::move(other.mDepthStencilAttachmentReference);
	}

	/**
	 * Links an attachment to this phase of a `RenderPass`.
	 */
	RenderPassPhase& addColorAttachment(RenderPassAttachment const &attachment);
	RenderPassPhase& setDepthAttachment(RenderPassAttachment const &attachment);

private:
	std::optional<uIndex> mIdxInRenderPass;

	// The attachment references for the first subpass.
	// The index of each element in this array directly relates to its location in the shader `layout(location = <index>) out <type> <name>`
	std::vector<vk::AttachmentReference> mColorAttachmentReferences;
	std::optional<vk::AttachmentReference> mDepthStencilAttachmentReference;

	vk::SubpassDescription description() const;

};

class RenderPass
{

public:
	struct DependencyItem
	{
		std::optional<RenderPassPhase> phase;
		vk::PipelineStageFlags stageFlags;
		vk::AccessFlags accessFlags;
	};

	RenderPass() = default;

	RenderPassAttachment& addAttachment(RenderPassAttachment &attachment);
	RenderPassPhase& addPhase(RenderPassPhase &phase);
	/**
	 * Can only be called once both the dependee phase and depender phase have been added via `addPhase`.
	 * Creates a dependency between two render pass phases such that dependee must happen before depender.
	 * If either `dependee` or `depender` is equivalent to `std::nullopt`, then it is assumed that the other dependency item
	 * is dependent on or is the dependee for the implicit external subpass/render phase.
	 * Both `dependee` and `depender` cannot be `std::nullopt`.
	 */
	RenderPass& addDependency(DependencyItem const dependee, DependencyItem const depender);

	RenderPass& create(std::shared_ptr<GraphicsDevice> device);
	bool isValid() const;
	void* get();
	void destroy();
	void reset();

private:
	std::vector<RenderPassAttachment> mAttachments;
	std::vector<RenderPassPhase> mPhases;
	std::vector<vk::SubpassDependency> mDependencies;

	vk::UniqueRenderPass mRenderPass;

};

NS_END
