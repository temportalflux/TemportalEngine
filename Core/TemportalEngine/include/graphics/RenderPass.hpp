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

	RenderPass& setImageFormatType(graphics::EImageFormatCategory type, ui32 vkImageFormat);
	ui32 getImageFormatFor(graphics::EImageFormatCategory type) const;

	RenderPass& setAttachments(std::vector<graphics::RenderPassAttachment> const& attachments);
	RenderPass& setPhases(std::vector<graphics::RenderPassPhase> const& phases);
	RenderPass& setPhaseDependencies(std::vector<graphics::RenderPassDependency> const& dependencies);

	bool isValid() const;
	void create() override;
	void* get() override;
	void invalidate() override;
	void resetConfiguration() override;

private:
	// TODO: These need to be used when performing a bind operation
	std::optional<math::Vector4> mClearColor;
	std::optional<std::pair<f32, ui32>> mClearDepthStencil;
	graphics::Area mRenderArea;

	std::vector<graphics::RenderPassAttachment> mAttachments;
	std::vector<graphics::RenderPassPhase> mPhases;
	std::vector<graphics::RenderPassDependency> mDependencies;

	std::unordered_map<graphics::EImageFormatCategory, ui32> mImageFormatsByType;

	vk::UniqueRenderPass mInternal;

};

NS_END
