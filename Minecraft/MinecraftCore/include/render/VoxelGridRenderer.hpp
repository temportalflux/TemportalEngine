#pragma once

#include "CoreInclude.hpp"
#include "graphics/Descriptor.hpp"
#include "asset/TypedAssetPath.hpp"

#include "BlockId.hpp"
#include "render/IPipelineRenderer.hpp"

FORWARD_DEF(NS_ASSET, class Pipeline);
FORWARD_DEF(NS_GAME, class VoxelTypeRegistry);
FORWARD_DEF(NS_GAME, class VoxelModelManager);
FORWARD_DEF(NS_GRAPHICS, class Pipeline);
FORWARD_DEF(NS_GRAPHICS, class DescriptorPool);
FORWARD_DEF(NS_WORLD, class BlockInstanceBuffer);

NS_GRAPHICS

class VoxelGridRenderer : public graphics::IPipelineRenderer, public std::enable_shared_from_this<VoxelGridRenderer>
{

public:
	VoxelGridRenderer(
		graphics::DescriptorPool *descriptorPool,
		std::weak_ptr<world::BlockInstanceBuffer> instanceBuffer,
		std::weak_ptr<game::VoxelTypeRegistry> registry,
		std::weak_ptr<game::VoxelModelManager> modelManager
	);
	~VoxelGridRenderer();
	
	std::function<void()> onAtlasesCreatedEvent();

	VoxelGridRenderer& setPipeline(asset::TypedAssetPath<asset::Pipeline> const& path);

	void setDevice(std::weak_ptr<graphics::GraphicsDevice> device) override;
	void setRenderPass(std::shared_ptr<graphics::RenderPass> renderPass) override;
	void setFrameCount(uSize frameCount) override {}
	void createDescriptors(graphics::DescriptorPool *descriptorPool) override {}
	void attachDescriptors(
		std::unordered_map<std::string, std::vector<graphics::Buffer*>> &mutableUniforms
	) override {}
	void writeDescriptors(std::shared_ptr<graphics::GraphicsDevice> device) override {}
	void setDescriptorLayouts(std::unordered_map<std::string, graphics::DescriptorLayout const*> const& globalLayouts) override;
	void createPipeline(math::Vector2UInt const& resolution) override;

	void record(graphics::Command *command, uIndex idxFrame, TGetGlobalDescriptorSet getGlobalDescriptorSet) override;

	void destroyRenderChain() override;
	void destroy();

private:
	graphics::DescriptorPool *mpDescriptorPool;
	std::weak_ptr<world::BlockInstanceBuffer> mpInstanceBuffer;
	std::weak_ptr<game::VoxelTypeRegistry> mpTypeRegistry;
	std::weak_ptr<game::VoxelModelManager> mpModelManager;

	std::shared_ptr<graphics::Pipeline> mpPipeline;
	graphics::DescriptorLayout mDescriptorLayoutTexture;
	std::vector<graphics::DescriptorSet> mAtlasDescriptors;
	std::unordered_map<game::BlockId, uIndex> mDescriptorSetIdxByVoxelId;

	void onAtlasesCreated();

};

NS_END
