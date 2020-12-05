#pragma once

#include "ecs/system/System.hpp"

#include "render/IPipelineRenderer.hpp"

FORWARD_DEF(NS_GRAPHICS, class SkinnedModelManager);

NS_ECS NS_SYSTEM

class RenderPlayer : public System, public graphics::IPipelineRenderer
{

public:
	RenderPlayer(
		std::weak_ptr<graphics::SkinnedModelManager> modelManager
	);

	void setDevice(std::weak_ptr<graphics::GraphicsDevice> device) override;
	void setRenderPass(std::shared_ptr<graphics::RenderPass> renderPass) override;
	void initializeData(graphics::CommandPool* transientPool) override;
	void setFrameCount(uSize frameCount) override;
	void createDescriptors(std::shared_ptr<graphics::GraphicsDevice> device) override;
	void attachDescriptors(
		std::unordered_map<std::string, std::vector<graphics::Buffer*>> &mutableUniforms
	) override;
	void writeDescriptors(std::shared_ptr<graphics::GraphicsDevice> device) override;
	void createPipeline(math::Vector2UInt const& resolution) override;
	
	void record(graphics::Command *command, uIndex idxFrame) override;
	void recordView(graphics::Command *command, std::shared_ptr<ecs::view::View> view);

	void destroyRenderChain() override;

	void update(f32 deltaTime, std::shared_ptr<ecs::view::View> view) override { assert(false); }

private:
	std::weak_ptr<graphics::SkinnedModelManager> const mpModelManager;

};

NS_END NS_END
