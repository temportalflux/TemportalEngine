#pragma once

#include "CoreInclude.hpp"
#include "graphics/DescriptorGroup.hpp"
#include "graphics/FontAtlas.hpp"

#include "render/IPipelineRenderer.hpp"

FORWARD_DEF(NS_ASSET, class Pipeline);
FORWARD_DEF(NS_ASSET, class Font);
FORWARD_DEF(NS_GRAPHICS, class Pipeline);
FORWARD_DEF(NS_GRAPHICS, class DescriptorPool);

NS_GRAPHICS

class UIRenderer : public graphics::IPipelineRenderer
{

public:
	UIRenderer(std::weak_ptr<graphics::DescriptorPool> pDescriptorPool);
	~UIRenderer();

	UIRenderer& setTextPipeline(std::shared_ptr<asset::Pipeline> asset);
	UIRenderer& addFont(std::string fontId, std::shared_ptr<asset::Font> asset);

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
	void destroyRenderChain() override;

	void destroyRenderDevices();
	
private:
	std::weak_ptr<graphics::DescriptorPool> mpDescriptorPool;

	struct FontFaceImage
	{
		uIndex idxDescriptor;
		graphics::Image image;
		graphics::ImageView view;
	};
	struct RegisteredFont
	{
		graphics::Font font;
		std::unordered_map<ui8, FontFaceImage> faces;
	};
	struct
	{
		std::shared_ptr<graphics::Pipeline> pipeline;
		std::vector<graphics::DescriptorGroup> descriptorGroups;
		graphics::ImageSampler sampler;
		std::shared_ptr<Memory> memoryFontImages;
		std::unordered_map<std::string, RegisteredFont> fonts;
		// the number of faces across all fonts
		ui8 fontFaceCount;
	} mText;

	void* getTextDescriptor(std::string const& fontId, ui8 fontSize) const;

};

NS_END
