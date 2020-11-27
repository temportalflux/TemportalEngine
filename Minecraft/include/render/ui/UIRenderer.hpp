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
	UIRenderer(
		std::weak_ptr<graphics::DescriptorPool> pDescriptorPool,
		uSize maximumDisplayedCharacters
	);
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
	struct FontGlyphVertex
	{
		/**
		 * The position of a text/string vertex.
		 * <x,y> is the position of the string screen space, where the range is [0,1].
		 * <z,w> is the font alignment offers from that position so the glyph is rendered correctly.
		 *		Derived from `FontGlyphMeta#metricsOffset` and `FontGlyphMeta#advance`.
		 * These two vec2s will be summed in the shader when rendering the vertex.
		 */
		math::Vector4 position;
		/**
		 * The coordinate of the glyph vertex in the font atlas allocated for a given `RegisteredFont`.
		 */
		math::Vector2 texCoord;
	};
	// Renderable data pertaining to a unique piece of text that is rendered.
	struct GlyphString
	{
		// The value of the text. This is converted using `fontId` and `fontSize` into `vertices`.
		std::string value;

		// The identifier of the `RegisteredFont`.
		std::string fontId;
		// The size of the font-face in `RegisteredFont`.
		ui8 fontSize;

		/**
		 * The vertex list for the string converted into glyphs.
		 * Each character in `value` has exactly 4 vertices in this list,  which composed into 2 triangles via `indices`.
		 */
		std::vector<FontGlyphVertex> vertices;
		/**
		 * The indices for the vertex buffer data in `vertices`.
		 * Each character in `value` has exactly 6 indices in this list (for the 2 triangles).
		 */
		std::vector<ui16> indices;
	};
	struct TextData
	{

		/**
		 * An ordered list of unique string ids, each which have a value in `textStrings`.
		 */
		std::set<std::string> stringIds;

		/**
		 * A map of string-id to the RenderableString data each represents.
		 */
		std::unordered_map<std::string, GlyphString> strings;

	};
	struct
	{
		// The pipeline used to render all text
		std::shared_ptr<graphics::Pipeline> pipeline;
		// The descriptors used to render any font. Specific descriptor sets are allocated for each registered font.
		std::vector<graphics::DescriptorGroup> descriptorGroups;
		// The sampler vulkan uses to read each font atlas
		graphics::ImageSampler sampler;
		// The memory that font atlases are allocated from
		std::shared_ptr<Memory> memoryFontImages;
		// A map of all fonts, their typefaces, and font sizes, including their atlases for each combination
		std::unordered_map<std::string, RegisteredFont> fonts;
		// the number of faces across all fonts
		ui8 fontFaceCount;

		TextData uncommittedData;
		bool bHasUncommittedDataChanges;
		TextData committedData;

		std::shared_ptr<Memory> memoryTextBuffers;
		graphics::Buffer vertexBuffer;
		graphics::Buffer indexBuffer;

	} mText;

	void* getTextDescriptor(std::string const& fontId, ui8 fontSize) const;

};

NS_END
