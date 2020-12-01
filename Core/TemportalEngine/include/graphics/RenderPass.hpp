#pragma once

#include "graphics/DeviceObject.hpp"

#include "graphics/FrameBuffer.hpp"
#include "graphics/ImageView.hpp"
#include "math/Vector.hpp"
#include "graphics/RenderPassMeta.hpp"
#include "graphics/Area.hpp"

#include <vector>
#include <vulkan/vulkan.hpp>

NS_GRAPHICS

class RenderPass : public DeviceObject
{

public:
	RenderPass() = default;

	RenderPass& setClearColor(std::optional<math::Vector4> const color);
	std::optional<math::Vector4> const& clearColor() const { return this->mClearColor; }
	RenderPass& setClearDepthStencil(std::optional<std::pair<f32, ui32>> const depthStencil);
	std::optional<std::pair<f32, ui32>> const& clearDepthStencil() const { return this->mClearDepthStencil; }
	RenderPass& setRenderArea(graphics::Area const area);
	graphics::Area const& renderArea() const { return this->mRenderArea; }

	RenderPass& setImageFormatType(graphics::ImageFormatReferenceType::Enum type, ui32 vkImageFormat);
	ui32 getImageFormatFor(graphics::ImageFormatReferenceType::Enum type) const;

	RenderPass& addPhase(graphics::RPPhase const &phase);
	RenderPass& addDependency(graphics::RPDependency const &dependency);

	bool isValid() const;
	void create() override;
	void* get() override;
	void invalidate() override;
	void resetConfiguration() override;

private:
	struct SubpassAttachments
	{
		std::vector<vk::AttachmentReference> color;
		std::optional<vk::AttachmentReference> depth;
	};

	// TODO: These need to be used when performing a bind operation
	std::optional<math::Vector4> mClearColor;
	std::optional<std::pair<f32, ui32>> mClearDepthStencil;
	graphics::Area mRenderArea;

	std::vector<graphics::RPPhase> mPhases;

	std::unordered_map<graphics::ImageFormatReferenceType::Enum, ui32> mImageFormatsByType;

	std::vector<vk::AttachmentDescription> mAttachments;
	std::vector<vk::SubpassDescription> mSubpasses;
	std::vector<SubpassAttachments> mSubpassAttachmentRefs;
	std::vector<vk::SubpassDependency> mDependencies;

	vk::UniqueRenderPass mInternal;

	void addPhaseAsSubpass(graphics::RPPhase const &phase);

};

NS_END
