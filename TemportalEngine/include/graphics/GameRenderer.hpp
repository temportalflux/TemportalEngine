#pragma once

#include "graphics/VulkanRenderer.hpp"

#include "graphics/AttributeBinding.hpp"
#include "graphics/DescriptorPool.hpp"
#include "graphics/DescriptorGroup.hpp"
#include "graphics/FontAtlas.hpp"
#include "graphics/Image.hpp"
#include "graphics/ImageSampler.hpp"
#include "graphics/ImageView.hpp"
#include "graphics/Memory.hpp"

class IRender;
FORWARD_DEF(NS_ASSET, class Font)
FORWARD_DEF(NS_ASSET, class Texture)
FORWARD_DEF(NS_ASSET, class TextureSampler)
FORWARD_DEF(NS_GRAPHICS, class StringRenderer)

NS_GRAPHICS
class Uniform;
class Image;
class Font;

/**
 * A Vulkan renderer tailored for rendering to a single surface using multiple view buffers.
 */
class GameRenderer : public VulkanRenderer
{

public:
	GameRenderer();

	void initializeDevices() override;
	std::shared_ptr<GraphicsDevice> getDevice();
	CommandPool& getTransientPool();

	void addRender(IRender *render);

	void setStaticUniform(std::shared_ptr<Uniform> uniform);
	
	void setBindings(std::vector<AttributeBinding> bindings);
	void addShader(std::shared_ptr<ShaderModule> shader);
	void setUIShaderBindings(std::shared_ptr<ShaderModule> shaderVert, std::shared_ptr<ShaderModule> shaderFrag, std::vector<AttributeBinding> bindings);

	// Creates an image sampler from some asset
	// TODO: Take in an asset object
	// Returns the idx of the sampler in `mTextureSamplers`
	uIndex createTextureSampler(std::shared_ptr<asset::TextureSampler> sampler);

	// Creates a `graphics::Image` from a `asset::Texture`.
	// Returns the idx of the image view in `mTextureViews`
	uIndex createTextureAssetImage(std::shared_ptr<asset::Texture> texture, uIndex idxSampler);
	// Must happen only after all image assets are created
	void allocateTextureMemory();
	void writeTextureData(uIndex idxTexture, std::shared_ptr<asset::Texture> texture);

	std::shared_ptr<StringRenderer> setFont(std::shared_ptr<asset::Font> font);
	void prepareUIBuffers(ui64 const maxCharCount);
	
	void createRenderChain() override;
	void createRenderPass() override;
	RenderPass* getRenderPass() override;
	void destroyRenderPass() override;

	void invalidate() override;
	std::shared_ptr<StringRenderer> stringRenderer();

protected:

	void createFrames(uSize viewCount) override;
	uSize getNumberOfFrames() const override;
	graphics::Frame* getFrameAt(uSize idx) override;
	void destroyFrames() override;

private:

	void destroyRenderChain() override;

	void createUniformBuffers();
	void createDepthResources(math::Vector2UInt const &resolution);
	void destroyDepthResources();
	void createDescriptors();
	void createCommandObjects();
	void destroyCommandObjects();
	void recordCommandBufferInstructions();

	void prepareRender(ui32 idxCurrentFrame) override;
	void updateUniformBuffer(ui32 idxImageView);
	void render(graphics::Frame* frame, ui32 idxCurrentImage) override;
	void onFramePresented(uIndex idxFrame) override;

private:

	CommandPool mCommandPoolTransient;
	std::vector<IRender*> mpRenders;

	RenderPass mRenderPass;

	// Pool for all descriptors that are used in this renderer
	DescriptorPool mDescriptorPool;

	std::shared_ptr<Uniform> mpUniformStatic; // used for global UBO like projection matrix
	std::shared_ptr<Memory> mpMemoryUniformBuffers;
	std::vector<Buffer> mUniformStaticBuffersPerFrame;
	
	std::shared_ptr<Memory> mpMemoryImages;
	std::vector<ImageSampler> mTextureSamplers;
	std::vector<Image> mTextureImages;
	std::vector<ImageView> mTextureViews;
	// First value of each pair is the image view idx of `mTextureViews`
	// Second value of each pair is the image sampler idx of `mTextureSamplers`
	std::vector<std::pair<uIndex, uIndex>> mTextureDescriptorPairs;
	std::shared_ptr<Memory> mpMemoryDepthImage;
	Image mDepthImage;
	ImageView mDepthView;

	std::vector<FrameBuffer> mFrameBuffers;

	DescriptorGroup mDescriptorGroup;
	Pipeline mPipeline;

	std::shared_ptr<StringRenderer> mpStringRenderer;
	std::shared_ptr<Memory> mpMemoryFontImages;
	DescriptorGroup mDescriptorGroupUI;
	Pipeline mPipelineUI;
	std::shared_ptr<Memory> mpMemoryUIBuffers;
	Buffer mVertexBufferUI, mIndexBufferUI;
	ui32 mIndexCountUI;

	CommandPool mCommandPool;
	std::vector<CommandBuffer> mCommandBuffers;

	std::vector<Frame> mFrames;

	void initializeTransientCommandPool();

};

NS_END
