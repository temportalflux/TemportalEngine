#pragma once

#include "CoreInclude.hpp"
#include "asset/TypedAssetPath.hpp"
#include "graphics/Buffer.hpp"
#include "graphics/Descriptor.hpp"
#include "graphics/FontAtlas.hpp"
#include "ui/ImageWidget.hpp"
#include "ui/WidgetRenderer.hpp"

#include "render/IPipelineRenderer.hpp"

FORWARD_DEF(NS_ASSET, class Pipeline);
FORWARD_DEF(NS_ASSET, class Font);
FORWARD_DEF(NS_GRAPHICS, class Pipeline);
FORWARD_DEF(NS_GRAPHICS, class DescriptorPool);

NS_GRAPHICS

class UIRenderer
	: public graphics::IPipelineRenderer
	, public ui::WidgetRenderer
	, public ui::FontOwner
{
	friend class UIString;

public:
	UIRenderer();
	~UIRenderer();

	UIRenderer& setImagePipeline(asset::TypedAssetPath<asset::Pipeline> const& path);
	UIRenderer& setTextPipeline(asset::TypedAssetPath<asset::Pipeline> const& path);
	UIRenderer& addFont(std::string fontId, std::shared_ptr<asset::Font> asset);

	void setDevice(std::weak_ptr<graphics::GraphicsDevice> device) override;
	void setRenderPass(std::shared_ptr<graphics::RenderPass> renderPass) override;
	void initializeData(graphics::CommandPool* transientPool, graphics::DescriptorPool *descriptorPool) override;
	void setFrameCount(uSize frameCount) override;
	void createDescriptors(graphics::DescriptorPool *descriptorPool) override;
	void attachDescriptors(
		std::unordered_map<std::string, std::vector<graphics::Buffer*>> &mutableUniforms
	) override;
	void writeDescriptors(std::shared_ptr<graphics::GraphicsDevice> device) override;
	void createPipeline(math::Vector2UInt const& resolution) override;
	void record(graphics::Command *command, uIndex idxFrame, TGetGlobalDescriptorSet getGlobalDescriptorSet) override;
	void destroyRenderChain() override;

	void destroyRenderDevices();

protected:

	graphics::Font const& getFont(std::string const& fontId) const override;
	
private:
	// A map of all fonts, their typefaces, and font sizes, including their atlases for each combination
	std::map<std::string, uIndex> mFontIds;
	std::vector<graphics::Font> mFonts;

};

NS_END
