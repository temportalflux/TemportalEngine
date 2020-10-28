#pragma once

#include "CoreInclude.hpp"
#include "graphics/VulkanRenderer.hpp"

#include "graphics/CommandPool.hpp"
#include "graphics/Image.hpp"
#include "graphics/ImageView.hpp"
#include "graphics/Memory.hpp"

FORWARD_DEF(NS_ASSET, class RenderPass)
FORWARD_DEF(NS_GRAPHICS, class IPipelineRenderer)
FORWARD_DEF(NS_GRAPHICS, class Uniform)

NS_GRAPHICS

class MinecraftRenderer : public VulkanRenderer
{

public:
	MinecraftRenderer();

	void initializeDevices() override;
	std::shared_ptr<GraphicsDevice> getDevice();
	CommandPool& getTransientPool();

	void addMutableUniform(std::string const& key, std::weak_ptr<Uniform> uniform);
	void addRenderer(IPipelineRenderer *renderer);
	void setRenderPass(std::shared_ptr<asset::RenderPass> renderPass);

	void createRenderChain() override;
	void finalizeInitialization() override;

	void invalidate() override;

protected:

	ui32 getSwapChainImageViewCount() const;

	void createRenderPass() override;
	RenderPass* getRenderPass() override;
	void destroyRenderPass() override;

	void createFrames(uSize viewCount) override;
	uSize getNumberOfFrames() const override;
	graphics::Frame* getFrameAt(uSize idx) override;
	void destroyFrames() override;

private:

	void destroyRenderChain() override;
	void createDepthResources(math::Vector2UInt const &resolution);
	void destroyDepthResources();

	void record(uIndex idxFrame);
	void prepareRender(ui32 idxCurrentFrame) override;
	void render(graphics::Frame* frame, ui32 idxCurrentImage) override;
	void onFramePresented(uIndex idxFrame) override;

private:
	CommandPool mCommandPoolTransient;
	
	std::unordered_map<std::string, std::weak_ptr<Uniform>> mpMutableUniforms;
	std::shared_ptr<Memory> mpMemoryUniformBuffers;

	std::vector<IPipelineRenderer*> mpRenderers;
	std::shared_ptr<graphics::RenderPass> mpRenderPass;

	std::shared_ptr<graphics::Memory> mpMemoryDepthImage;
	graphics::Image mDepthImage;
	graphics::ImageView mDepthView;

	struct CommandBufferFrame
	{
		std::unordered_map<std::string, graphics::Buffer> uniformBuffers;
		graphics::CommandPool commandPool;
		graphics::CommandBuffer commandBuffer;
		graphics::Frame frame;
		graphics::FrameBuffer frameBuffer;
	};
	std::vector<CommandBufferFrame> mFrames;

	void initializeTransientCommandPool();

};

NS_END