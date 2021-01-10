#pragma once

#include "CoreInclude.hpp"
#include "graphics/VulkanRenderer.hpp"

#include "graphics/CommandPool.hpp"
#include "graphics/DescriptorPool.hpp"
#include "graphics/Image.hpp"
#include "graphics/ImageView.hpp"

FORWARD_DEF(NS_ASSET, class RenderPass)
FORWARD_DEF(NS_GRAPHICS, class IPipelineRenderer)
FORWARD_DEF(NS_GRAPHICS, class Uniform)

NS_GRAPHICS

class MinecraftRenderer : public VulkanRenderer
{

public:
	MinecraftRenderer();

	SimpleExecuteDelegate UpdateWorldGraphicsOnFramePresented;

	void initializeDevices() override;
	CommandPool& getTransientPool();
	DescriptorPool& getDescriptorPool();
	ui32 getSwapChainImageViewCount() const;

	void addGlobalMutableDescriptor(std::string const& layoutId, uSize const& bindingCount);
	void addMutableUniform(std::string const& key, std::weak_ptr<Uniform> uniform);
	void addMutableUniformToLayout(
		std::string const& layoutId, std::string const& uniformId,
		uIndex const& bindingIndex,
		graphics::DescriptorType const& type, graphics::ShaderStage const& stage
	);
	void addRenderer(IPipelineRenderer *renderer);
	void setRenderPass(std::shared_ptr<asset::RenderPass> renderPass);

	void setDPI(ui32 dotsPerInch);
	ui32 dpi() const;

	void createMutableUniforms();
	void createRenderChain() override;
	void finalizeInitialization() override;

	void invalidate() override;

protected:

	void createRenderPass() override;
	RenderPass* getRenderPass() override;
	void destroyRenderPass() override;

	void createFrames(uSize viewCount) override;
	uSize getNumberOfFrames() const override;
	graphics::Frame* getFrameAt(uSize idx) override;
	void destroyFrames() override;

private:

	void createMutableUniformBuffers();
	void destroyRenderChain() override;
	void createDepthResources(math::Vector2UInt const &resolution);
	void destroyDepthResources();

	void record(uIndex idxFrame);
	void prepareRender(ui32 idxCurrentFrame) override;
	void render(graphics::Frame* frame, ui32 idxCurrentImage) override;
	void onFramePresented(uIndex idxFrame) override;

private:
	CommandPool mCommandPoolTransient;
	DescriptorPool mGlobalDescriptorPool;
	
	std::unordered_map<std::string, std::weak_ptr<Uniform>> mpMutableUniforms;
	std::unordered_map<std::string, std::vector<graphics::Buffer*>> mMutableUniformBuffersByDescriptorId;
	struct GlobalMutableDescriptor
	{
		std::vector<std::string> mutableUniformIds;
		graphics::DescriptorLayout layout;
		std::vector<graphics::DescriptorSet> sets;
	};
	std::unordered_map<std::string, GlobalMutableDescriptor> mGlobalMutableDescriptors;

	std::vector<IPipelineRenderer*> mpRenderers;
	std::shared_ptr<graphics::RenderPass> mpRenderPass;
	ui32 mDPI;

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
	std::unordered_map<std::string, graphics::DescriptorLayout const*> getGlobalDescriptorLayouts() const;
	graphics::DescriptorSet const* getGlobalDescriptorSet(std::string const& layoutId, uIndex idxFrame) const;

};

NS_END
