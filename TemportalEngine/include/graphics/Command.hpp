#pragma once

#include "TemportalEnginePCH.hpp"

#include "types/integer.h"
#include "types/real.h"

#include <array>
#include <vector>
#include <optional>
#include <vulkan/vulkan.hpp>

NS_GRAPHICS
class CommandBuffer;
class RenderPass;
class FrameBuffer;
class Pipeline;
class Buffer;

class Command
{
	friend class CommandBuffer;

public:

	// TODO: pass engine vector instead of std array
	Command& clear(std::array<f32, 4U> color);

	Command& beginRenderPass(RenderPass const *pRenderPass, FrameBuffer const *pFrameBuffer);
	Command& bindPipeline(Pipeline const *pPipeline);
	Command& bindVertexBuffers(std::vector<Buffer*> const pBuffers);
	Command& bindIndexBuffer(ui64 offset, Buffer* const pBuffer, vk::IndexType indexType);
	Command& draw(ui32 vertexCount);
	Command& endRenderPass();
	
	// For copying transfer buffers
	Command& copyBuffer(Buffer *src, Buffer *dest, ui64 size);

	void end();

private:
	CommandBuffer *mpBuffer;
	std::optional<vk::ClearValue> mClearValue;
	
	Command(CommandBuffer *pBuffer);


};

NS_END
