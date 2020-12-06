#pragma once

#include "ecs/system/System.hpp"

#include "asset/PipelineAsset.hpp"
#include "graphics/Descriptor.hpp"
#include "render/IPipelineRenderer.hpp"

FORWARD_DEF(NS_GRAPHICS, class SkinnedModelManager);
FORWARD_DEF(NS_GRAPHICS, class Pipeline);

NS_ECS NS_SYSTEM

class RenderPlayer : public System, public graphics::IPipelineRenderer
{

public:
	RenderPlayer(
		std::weak_ptr<graphics::SkinnedModelManager> modelManager,
		std::weak_ptr<graphics::DescriptorPool> globalDescriptorPool
	);
	~RenderPlayer();

	RenderPlayer& setPipeline(asset::TypedAssetPath<asset::Pipeline> const& path);

	void setDevice(std::weak_ptr<graphics::GraphicsDevice> device) override;
	void setRenderPass(std::shared_ptr<graphics::RenderPass> renderPass) override;
	void setFrameCount(uSize frameCount) override;
	void initializeData(graphics::CommandPool* transientPool) override;
	void createDescriptors(std::shared_ptr<graphics::GraphicsDevice> device) override;
	void attachDescriptors(
		std::unordered_map<std::string, std::vector<graphics::Buffer*>> &mutableUniforms
	) override;
	void writeDescriptors(std::shared_ptr<graphics::GraphicsDevice> device) override;
	void createPipeline(math::Vector2UInt const& resolution) override;
	
	void record(graphics::Command *command, uIndex idxFrame) override;
	void recordView(graphics::Command *command, uIndex idxFrame, std::shared_ptr<ecs::view::View> view);

	void destroyRenderChain() override;
	void destroy();

	void update(f32 deltaTime, std::shared_ptr<ecs::view::View> view) override;

private:
	std::weak_ptr<graphics::SkinnedModelManager> const mpModelManager;
	std::weak_ptr<graphics::DescriptorPool> const mpDescriptorPool;

	std::shared_ptr<graphics::Pipeline> mpPipeline;
	graphics::DescriptorLayout mDescriptorLayout;
	// TODO: This will need to be a per-entity thing that gets pooled so that each entity can use its own texture
	std::vector<graphics::DescriptorSet> mDescriptorSets;

};

NS_END NS_END
