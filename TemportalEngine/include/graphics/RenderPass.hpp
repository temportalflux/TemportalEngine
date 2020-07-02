#pragma once

#include "TemportalEnginePCH.hpp"

#include "graphics/FrameBuffer.hpp"
#include "graphics/ImageView.hpp"
#include "math/Vector.hpp"

#include <vector>
#include <vulkan/vulkan.hpp>

NS_GRAPHICS
class LogicalDevice;

class RenderPass
{

public:
	RenderPass();

	RenderPass& setFormat(ui32 const formatValue);
	// Sets the portion of the viewport to render this pass to
	RenderPass& setScissorBounds(math::Vector2Int const offset, math::Vector2UInt const resolution);

	math::Vector2Int const& getScissorOffset() const;
	math::Vector2UInt const& getScissorResolution() const;

	void* get();

	bool isValid() const;
	RenderPass& create(LogicalDevice *pDevice, std::optional<vk::Format> depthBufferFormat);
	void destroy();

	vk::RenderPass getRenderPass() const;

private:
	ui32 mFormatValue;
	math::Vector2Int mScissorOffset;
	math::Vector2UInt mScissorResolution;

	vk::UniqueRenderPass mRenderPass;

};

NS_END
