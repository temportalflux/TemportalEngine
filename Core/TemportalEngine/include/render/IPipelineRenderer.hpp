#pragma once

#include "TemportalEnginePCH.hpp"

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

	virtual void setDevice(std::weak_ptr<graphics::GraphicsDevice> device) {}
	virtual void setRenderPass(std::shared_ptr<graphics::RenderPass> renderPass) {}
	virtual void initializeData(graphics::CommandPool* transientPool, graphics::DescriptorPool *descriptorPool) {};
	virtual void setFrameCount(uSize frameCount) {}
	virtual void createDescriptors(graphics::DescriptorPool *descriptorPool) {}
	virtual void attachDescriptors(
		std::unordered_map<std::string, std::vector<graphics::Buffer*>> &mutableUniforms
	) {}
	virtual void writeDescriptors(std::shared_ptr<graphics::GraphicsDevice> device) {}
	virtual void setDescriptorLayouts(std::unordered_map<std::string, graphics::DescriptorLayout const*> const& globalLayouts) {}
	virtual void createPipeline(math::Vector2UInt const& resolution) {}
	virtual void record(graphics::Command *command, uIndex idxFrame, TGetGlobalDescriptorSet getGlobalDescriptorSet) {}
	virtual void destroyRenderChain() {}
};

NS_END
