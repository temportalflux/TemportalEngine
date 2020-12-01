#pragma once

#include "CoreInclude.hpp"

#include "render/IPipelineRenderer.hpp"
#include "graphics/AttributeBinding.hpp"
#include "graphics/DescriptorGroup.hpp"
#include "graphics/Buffer.hpp"

FORWARD_DEF(NS_ASSET, class Pipeline);
FORWARD_DEF(NS_GRAPHICS, class DescriptorPool);
FORWARD_DEF(NS_GRAPHICS, class Pipeline);
FORWARD_DEF(NS_GRAPHICS, class CommandPool);

NS_GRAPHICS

class LineRenderer : public graphics::IPipelineRenderer
{

public:
	LineRenderer(std::weak_ptr<graphics::DescriptorPool> pDescriptorPool);
	~LineRenderer();

	LineRenderer& setPipeline(std::shared_ptr<asset::Pipeline> asset);
	void createGraphicsBuffers(graphics::CommandPool* transientPool);

	// ~~~~~~~~~~ START: IPipelineRenderer ~~~~~~~~~~
	void setDevice(std::weak_ptr<graphics::GraphicsDevice> device);
	void setRenderPass(std::shared_ptr<graphics::RenderPass> renderPass);
	void setFrameCount(uSize frameCount);
	void createDescriptors(std::shared_ptr<graphics::GraphicsDevice> device);
	virtual void attachDescriptors(
		std::unordered_map<std::string, std::vector<graphics::Buffer*>> &mutableUniforms
	);
	void writeDescriptors(std::shared_ptr<graphics::GraphicsDevice> device);
	void createPipeline(math::Vector2UInt const& resolution);
	void record(graphics::Command *command, uIndex idxFrame);
	void destroyRenderChain();
	// ~~~~~~~~~~~~ END: IPipelineRenderer ~~~~~~~~~~

	void destroyBuffers();

protected:

	std::vector<graphics::DescriptorGroup> mDescriptorGroups;

	virtual graphics::AttributeBinding makeVertexBinding(ui8 &slot) const = 0;
	virtual uSize vertexBufferSize() const = 0;
	virtual void* vertexBufferData() const = 0;
	virtual ui32 indexCount() const = 0;
	virtual uSize indexBufferSize() const = 0;
	virtual void* indexBufferData() const = 0;

private:

	std::weak_ptr<graphics::DescriptorPool> mpDescriptorPool;
	std::shared_ptr<graphics::Pipeline> mpPipeline;
	graphics::Buffer mVertexBuffer, mIndexBuffer;

	virtual void draw(graphics::Command *command);

};

NS_END
