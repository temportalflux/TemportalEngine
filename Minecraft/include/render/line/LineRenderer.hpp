#pragma once

#include "CoreInclude.hpp"

#include "render/IPipelineRenderer.hpp"
#include "graphics/DescriptorGroup.hpp"
#include "graphics/Buffer.hpp"

FORWARD_DEF(NS_ASSET, class Pipeline);
FORWARD_DEF(NS_GRAPHICS, class DescriptorPool);
FORWARD_DEF(NS_GRAPHICS, class Pipeline);
FORWARD_DEF(NS_GRAPHICS, class CommandPool);

NS_GRAPHICS

struct LineSegment
{
	math::Vector3Padded pos1;
	math::Vector3Padded pos2;
	math::Vector4 color;
};

class LineRenderer : public graphics::IPipelineRenderer
{

public:
	LineRenderer(std::weak_ptr<graphics::DescriptorPool> pDescriptorPool);
	~LineRenderer();

	LineRenderer& setPipeline(std::shared_ptr<asset::Pipeline> asset);
	ui32 addLineSegment(LineSegment const& segment);
	void createGraphicsBuffers(graphics::CommandPool* transientPool);

	// ~~~~~~~~~~ START: IPipelineRenderer ~~~~~~~~~~
	void setDevice(std::weak_ptr<graphics::GraphicsDevice> device);
	void setRenderPass(std::shared_ptr<graphics::RenderPass> renderPass);
	void setFrameCount(uSize frameCount);
	void createDescriptors(std::shared_ptr<graphics::GraphicsDevice> device);
	void attachDescriptors(
		std::unordered_map<std::string, std::vector<graphics::Buffer*>> &mutableUniforms
	);
	void writeDescriptors(std::shared_ptr<graphics::GraphicsDevice> device);
	void createPipeline(math::Vector2UInt const& resolution);
	void record(graphics::Command *command, uIndex idxFrame);
	void destroyRenderChain();
	// ~~~~~~~~~~~~ END: IPipelineRenderer ~~~~~~~~~~

	void destroyBuffers();

protected:

	ui32 indexCount() const;

private:

	struct LineVertex
	{
		math::Vector3Padded position;
		math::Vector4 color;
	};

	std::weak_ptr<graphics::DescriptorPool> mpDescriptorPool;
	std::shared_ptr<graphics::Pipeline> mpPipeline;
	std::vector<graphics::DescriptorGroup> mDescriptorGroups;
	std::shared_ptr<graphics::Memory> mpMemoryGraphicsBuffers;
	graphics::Buffer mVertexBuffer, mIndexBuffer;

	std::vector<LineVertex> mVerticies;
	std::vector<ui16> mIndicies;
	ui16 pushVertex(LineVertex vertex);

	virtual void draw(graphics::Command *command);

};

NS_END
