#pragma once

#include "graphics/VulkanRenderer.hpp"

#include "graphics/AttributeBinding.hpp"

class IRender;
FORWARD_DEF(NS_ASSET, class Texture)

NS_GRAPHICS
class Uniform;
class Image;

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
	void writeBufferData(graphics::Buffer &buffer, uSize offset, std::vector<TData> const &dataSet)
	{
		this->writeToBuffer(&buffer, offset, (void*)dataSet.data(), sizeof(TData) * (uSize)dataSet.size());
	}
	
	void setBindings(std::vector<AttributeBinding> bindings);
	void addShader(std::shared_ptr<ShaderModule> shader);

	// Creates a `graphics::Image` from a `asset::Texture`
	Image& createTextureAssetImage(std::shared_ptr<asset::Texture> texture);

	void createRenderChain() override;

	void invalidate() override;

protected:

	void createFrames(uSize viewCount) override;
	uSize getNumberOfFrames() const override;
	graphics::Frame* getFrameAt(uSize idx) override;
	void destroyFrames() override;

private:

	void writeToBuffer(Buffer* buffer, uSize offset, void* data, uSize size);
	void copyBetweenBuffers(Buffer *src, Buffer *dest, uSize size);
	void copyBufferToImage(Buffer *src, Image *dest);
	void transitionImageToLayout(Image *image, vk::ImageLayout prev, vk::ImageLayout next);

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
