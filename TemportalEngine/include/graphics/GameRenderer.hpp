#pragma once

#include "graphics/VulkanRenderer.hpp"

NS_GRAPHICS

/**
 * A Vulkan renderer tailored for rendering to a single surface using multiple view buffers.
 */
class GameRenderer : public VulkanRenderer
{

public:
	GameRenderer();

	void addUniform(std::shared_ptr<Uniform> uniform);
	void createInputBuffers(ui64 vertexBufferSize, ui64 indexBufferSize);

	void createRenderChain() override;

protected:

	void createFrames(uSize viewCount) override;
	uSize getNumberOfFrames() const override;
	graphics::Frame* getFrameAt(uSize idx) override;
	void destroyFrames() override;

private:

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

private:

	std::vector<Frame> mFrames;

};

NS_END
