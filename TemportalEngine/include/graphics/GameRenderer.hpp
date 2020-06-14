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

	void createRenderChain() override;
	//void createInputBuffers(ui64 vertexBufferSize, ui64 indexBufferSize) override {}

protected:
	void destroyRenderChain() override;

	void createFrames(uSize viewCount) override;
	uSize getNumberOfFrames() const override;
	graphics::Frame* getFrameAt(uSize idx) override;
	void destroyFrames() override;

private:
	void createUniformBuffers() override;
	void destroyUniformBuffers() override;
	void createDescriptorPool() override;
	void destroyDescriptorPool() override;
	void createCommandObjects() override;
	void destroyCommandObjects() override;
	void recordCommandBufferInstructions();

	// TODO: Move these from vulkan to game
	//void destroyInputBuffers() override {}
	//void updateUniformBuffer(ui32 idxImageView) override {}

private:

	std::vector<Frame> mFrames;

};

NS_END
