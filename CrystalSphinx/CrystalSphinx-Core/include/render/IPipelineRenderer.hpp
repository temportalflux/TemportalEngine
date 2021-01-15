#pragma once

#include "CoreInclude.hpp"

FORWARD_DEF(NS_GRAPHICS, class GraphicsDevice);
FORWARD_DEF(NS_GRAPHICS, class RenderPass);
FORWARD_DEF(NS_GRAPHICS, class Buffer);
FORWARD_DEF(NS_GRAPHICS, class Command);
FORWARD_DEF(NS_GRAPHICS, class CommandPool);
FORWARD_DEF(NS_GRAPHICS, class DescriptorPool);
FORWARD_DEF(NS_GRAPHICS, class DescriptorSet);
FORWARD_DEF(NS_GRAPHICS, class DescriptorLayout);

NS_GRAPHICS

class IPipelineRenderer
{
public:
	typedef std::function<graphics::DescriptorSet const*(std::string const& layoutId, uIndex idxFrame)> TGetGlobalDescriptorSet;

	virtual void setDevice(std::weak_ptr<graphics::GraphicsDevice> device) = 0;
	virtual void setRenderPass(std::shared_ptr<graphics::RenderPass> renderPass) = 0;
	virtual void initializeData(graphics::CommandPool* transientPool, graphics::DescriptorPool *descriptorPool) {};
	virtual void setFrameCount(uSize frameCount) = 0;
	virtual void createDescriptors(graphics::DescriptorPool *descriptorPool) = 0;
	virtual void attachDescriptors(
		std::unordered_map<std::string, std::vector<graphics::Buffer*>> &mutableUniforms
	) = 0;
	virtual void writeDescriptors(std::shared_ptr<graphics::GraphicsDevice> device) = 0;
	virtual void setDescriptorLayouts(std::unordered_map<std::string, graphics::DescriptorLayout const*> const& globalLayouts) {}
	virtual void createPipeline(math::Vector2UInt const& resolution) = 0;
	virtual void record(graphics::Command *command, uIndex idxFrame, TGetGlobalDescriptorSet getGlobalDescriptorSet) = 0;
	virtual void destroyRenderChain() = 0;
};

NS_END
