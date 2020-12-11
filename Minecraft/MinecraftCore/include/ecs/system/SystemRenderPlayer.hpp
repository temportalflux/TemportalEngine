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
		graphics::DescriptorPool *descriptorPool
	);
	~RenderPlayer();

	RenderPlayer& setPipeline(asset::TypedAssetPath<asset::Pipeline> const& path);
	void createLocalPlayerDescriptor();

	void setDevice(std::weak_ptr<graphics::GraphicsDevice> device) override;
	void setRenderPass(std::shared_ptr<graphics::RenderPass> renderPass) override;
	void setFrameCount(uSize frameCount) override;
	void initializeData(graphics::CommandPool* transientPool, graphics::DescriptorPool *descriptorPool) override;
	void createDescriptors(graphics::DescriptorPool *descriptorPool) override;
	void attachDescriptors(
		std::unordered_map<std::string, std::vector<graphics::Buffer*>> &mutableUniforms
	) override;
	void writeDescriptors(std::shared_ptr<graphics::GraphicsDevice> device) override;
	void setDescriptorLayouts(std::unordered_map<std::string, graphics::DescriptorLayout const*> const& globalLayouts) override;
	void createPipeline(math::Vector2UInt const& resolution) override;
	
	void record(graphics::Command *command, uIndex idxFrame, TGetGlobalDescriptorSet getGlobalDescriptorSet) override;
	void recordView(graphics::Command *command, graphics::DescriptorSet const* cameraSet, std::shared_ptr<ecs::view::View> view);

	void destroyRenderChain() override;
	void destroy();

	void update(f32 deltaTime, std::shared_ptr<ecs::view::View> view) override;

private:
	std::weak_ptr<graphics::SkinnedModelManager> const mpModelManager;
	
	std::shared_ptr<graphics::Pipeline> mpPipeline;

	std::shared_ptr<graphics::DescriptorSetPool> mpModelDescriptors;
	graphics::DescriptorSetPool::Handle mModelDescriptor_DefaultHumanoid;

};

NS_END NS_END
