#pragma once

#include "CoreInclude.hpp"
#include "graphics/DescriptorGroup.hpp"

#include "BlockId.hpp"
#include "render/IPipelineRenderer.hpp"

FORWARD_DEF(NS_ASSET, class Pipeline);
FORWARD_DEF(NS_GAME, class VoxelTypeRegistry);
FORWARD_DEF(NS_GAME, class VoxelModelManager);
FORWARD_DEF(NS_GRAPHICS, class Pipeline);
FORWARD_DEF(NS_GRAPHICS, class DescriptorPool);
FORWARD_DEF(NS_WORLD, class BlockInstanceBuffer);

NS_GRAPHICS

class VoxelGridRenderer : public graphics::IPipelineRenderer
{

public:
	VoxelGridRenderer(
		std::weak_ptr<graphics::DescriptorPool> pDescriptorPool,
		std::weak_ptr<world::BlockInstanceBuffer> instanceBuffer
	);
	~VoxelGridRenderer();

	VoxelGridRenderer& setPipeline(std::shared_ptr<asset::Pipeline> asset);

	void setDevice(std::weak_ptr<graphics::GraphicsDevice> device) override;
	void setRenderPass(std::shared_ptr<graphics::RenderPass> renderPass) override;
	void createVoxelDescriptorMapping(
		std::shared_ptr<game::VoxelTypeRegistry> registry,
		std::shared_ptr<game::VoxelModelManager> modelManager
	);
	void setFrameCount(uSize frameCount) override;
	void createDescriptors(std::shared_ptr<graphics::GraphicsDevice> device) override;
	void attachDescriptors(
		std::unordered_map<std::string, std::vector<graphics::Buffer*>> &mutableUniforms
	) override;
	void writeDescriptors(std::shared_ptr<graphics::GraphicsDevice> device) override;
	void createPipeline(math::Vector2UInt const& resolution) override;

	void record(graphics::Command *command, uIndex idxFrame) override;

	void destroyRenderChain() override;
	void destroyRenderDevices();

private:
	std::weak_ptr<game::VoxelTypeRegistry> mpTypeRegistry;
	std::weak_ptr<game::VoxelModelManager> mpModelManager;
	std::weak_ptr<world::BlockInstanceBuffer> mpInstanceBuffer;

	std::shared_ptr<graphics::Pipeline> mpPipeline;
	std::weak_ptr<graphics::DescriptorPool> mpDescriptorPool;
	std::vector<graphics::DescriptorGroup> mDescriptorGroups;
	std::unordered_map<game::BlockId, uIndex /*descriptor archetype idx*/> mVoxelIdToDescriptorArchetype;

};

NS_END
