#pragma once

#include "graphics/VulkanRenderer.hpp"

#include "graphics/AttributeBinding.hpp"

class IRender;

NS_GRAPHICS
class Uniform;

/**
 * A Vulkan renderer tailored for rendering to a single surface using multiple view buffers.
 */
class GameRenderer : public VulkanRenderer
{

public:
	GameRenderer();

	void addRender(IRender *render);

	void setStaticUniform(std::shared_ptr<Uniform> uniform);
	void initializeBufferHelpers();
	void initializeBuffer(graphics::Buffer &buffer);

	// TODO: Move this to the buffer object
	template <typename TData>
	void writeBufferData(graphics::Buffer &buffer, ui64 offset, std::vector<TData> const &dataSet)
	{
		this->writeToBuffer(&buffer, offset, (void*)dataSet.data(), sizeof(TData) * dataSet.size());
	}
	
	void setBindings(std::vector<AttributeBinding> bindings);
	void addShader(std::shared_ptr<ShaderModule> shader);

	void createRenderChain() override;

	void invalidate() override;

protected:

	void createFrames(uSize viewCount) override;
	uSize getNumberOfFrames() const override;
	graphics::Frame* getFrameAt(uSize idx) override;
	void destroyFrames() override;

private:

	void writeToBuffer(Buffer* buffer, ui64 offset, void* data, ui64 size);
	void copyBetweenBuffers(Buffer *src, Buffer *dest, ui64 size);

	void destroyRenderChain() override;

	void createUniformBuffers();
	void destroyUniformBuffers();
	void createDescriptorPool();
	void destroyDescriptorPool();
	void createCommandObjects();
	void destroyCommandObjects();
	void recordCommandBufferInstructions();

	void prepareRender(ui32 idxCurrentFrame) override;
	void updateUniformBuffer(ui32 idxImageView);
	void render(graphics::Frame* frame, ui32 idxCurrentImage) override;

private:

	CommandPool mCommandPoolTransient;
	std::vector<IRender*> mpRenders;

	std::shared_ptr<Uniform> mpUniformStatic; // used for global UBO like projection matrix
	std::vector<Buffer> mUniformStaticBuffersPerFrame;
	vk::UniqueDescriptorPool mDescriptorPool_StaticUniform;
	vk::UniqueDescriptorSetLayout mDescriptorLayout_StaticUniform;
	std::vector<vk::DescriptorSet> mDescriptorSetPerFrame_StaticUniform;

	void createDescriptors(
		vk::DescriptorType type, vk::ShaderStageFlags stage,
		std::vector<Buffer> &bufferPerFrame, ui64 bufferRange,
		vk::UniqueDescriptorPool *pool, vk::UniqueDescriptorSetLayout *layout, std::vector<vk::DescriptorSet> &sets
	);

	std::vector<FrameBuffer> mFrameBuffers;
	Pipeline mPipeline;
	CommandPool mCommandPool;
	std::vector<CommandBuffer> mCommandBuffers;

	std::vector<Frame> mFrames;

};

NS_END
