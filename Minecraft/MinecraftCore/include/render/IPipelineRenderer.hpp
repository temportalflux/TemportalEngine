#pragma once

#include "CoreInclude.hpp"

FORWARD_DEF(NS_GRAPHICS, class GraphicsDevice);
FORWARD_DEF(NS_GRAPHICS, class RenderPass);
FORWARD_DEF(NS_GRAPHICS, class Buffer);
FORWARD_DEF(NS_GRAPHICS, class Command);
FORWARD_DEF(NS_GRAPHICS, class CommandPool);

NS_GRAPHICS

class IPipelineRenderer
{
public:
	virtual void setDevice(std::weak_ptr<graphics::GraphicsDevice> device) = 0;
	virtual void setRenderPass(std::shared_ptr<graphics::RenderPass> renderPass) = 0;
	virtual void initializeData(graphics::CommandPool* transientPool) {};
	virtual void setFrameCount(uSize frameCount) = 0;
	virtual void createDescriptors(std::shared_ptr<graphics::GraphicsDevice> device) = 0;
	virtual void attachDescriptors(
		std::unordered_map<std::string, std::vector<graphics::Buffer*>> &mutableUniforms
	) = 0;
	virtual void writeDescriptors(std::shared_ptr<graphics::GraphicsDevice> device) = 0;
	virtual void createPipeline(math::Vector2UInt const& resolution) = 0;
	virtual void record(graphics::Command *command, uIndex idxFrame) = 0;
	virtual void destroyRenderChain() = 0;
};

NS_END
