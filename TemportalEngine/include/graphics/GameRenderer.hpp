#pragma once

#include "graphics/VulkanRenderer.hpp"

NS_GRAPHICS
class Uniform;

/**
 * A Vulkan renderer tailored for rendering to a single surface using multiple view buffers.
 */
class GameRenderer : public VulkanRenderer
{

public:
	GameRenderer();

	void addUniform(std::shared_ptr<Uniform> uniform);
	void createInputBuffers(ui64 vertexBufferSize, ui64 indexBufferSize);

	template <typename TVertex>
	void writeVertexData(ui64 offset, std::vector<TVertex> verticies)
	{
		this->writeToBuffer(&this->mVertexBuffer, offset, (void*)verticies.data(), sizeof(TVertex) * verticies.size());
	}

	void writeIndexData(ui64 offset, std::vector<ui16> indicies)
	{
		this->mIndexBufferUnitType = vk::IndexType::eUint16;
		this->mIndexCount = (ui32)indicies.size();
		this->writeToBuffer(&this->mIndexBuffer, offset, (void*)indicies.data(), sizeof(ui16) * indicies.size());
	}

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
	void destroyInputBuffers();

	void destroyRenderChain() override;

	void createUniformBuffers();
	void destroyUniformBuffers();
	void createDescriptorPool();
	void destroyDescriptorPool();
	void createCommandObjects();
	void destroyCommandObjects();
	void recordCommandBufferInstructions();

	void prepareRender() override;
	void updateUniformBuffer(ui32 idxImageView);
	void render() override;

private:

	CommandPool mCommandPoolTransient;
	ui32 mIndexCount;
	Buffer mVertexBuffer;
	Buffer mIndexBuffer;
	vk::IndexType mIndexBufferUnitType;

	std::vector<std::shared_ptr<Uniform>> mUniformPts;
	ui64 mTotalUniformSize;
	std::vector<Buffer> mUniformBuffers;
	vk::UniqueDescriptorPool mDescriptorPool;
	vk::UniqueDescriptorSetLayout mDescriptorLayout;
	std::vector<vk::DescriptorSet> mDescriptorSets;

	std::vector<FrameBuffer> mFrameBuffers;
	Pipeline mPipeline;
	CommandPool mCommandPool;
	std::vector<CommandBuffer> mCommandBuffers;

	std::vector<Frame> mFrames;

};

NS_END
