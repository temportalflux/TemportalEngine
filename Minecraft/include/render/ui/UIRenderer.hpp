#pragma once

#include "CoreInclude.hpp"
#include "graphics/DescriptorGroup.hpp"
#include "graphics/FontAtlas.hpp"
#include "thread/MutexLock.hpp"

#include "render/IPipelineRenderer.hpp"

FORWARD_DEF(NS_ASSET, class Pipeline);
FORWARD_DEF(NS_ASSET, class Font);
FORWARD_DEF(NS_GRAPHICS, class Pipeline);
FORWARD_DEF(NS_GRAPHICS, class DescriptorPool);
FORWARD_DEF(NS_GRAPHICS, class UIString);

NS_GRAPHICS

class UIRenderer : public graphics::IPipelineRenderer
{
	friend class UIString;

public:
	UIRenderer(
		std::weak_ptr<graphics::DescriptorPool> pDescriptorPool,
		uSize maximumDisplayedCharacters
	);
	~UIRenderer();

	void lock();
	void unlock();
	bool hasChanges() const;

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

	/**
	 * Writes any changes made by `updateGlyphVertices` to the GPU buffer object.
	 */
	void commitToBuffer(graphics::CommandPool* transientPool);

protected:

	void addString(std::shared_ptr<UIString> pStr);
	void removeString(UIString const* pStr);
	void updateString(UIString const* pStr);
	math::Vector2 measure(UIString const* pStr) const;
	
private:
	std::weak_ptr<graphics::DescriptorPool> mpDescriptorPool;

	math::Vector2UInt mScreenResolution;
	thread::MutexLock mMutex;

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
		math::Vector2Padded texCoord;
	};
	// Renderable data pertaining to a unique piece of text that is rendered.
	// Generated from a `UIString`.
	struct GlyphString
	{
		std::shared_ptr<UIString> handle;

		struct
		{
			math::Vector2 position;
			std::string content;
			std::string fontId;
			ui8 fontSize;
		} prev;

		/**
		 * The vertex list for the string converted into glyphs.
		 * Each character in `content` has exactly 4 vertices in this list,  which composed into 2 triangles via `indices`.
		 */
		std::vector<FontGlyphVertex> vertices;

		/**
		 * The indices for the vertex buffer data in `vertices`.
		 * Each character in `content` has exactly 6 indices in this list (for the 2 triangles).
		 */
		std::vector<ui16> indices;
	};
	struct UncommittedTextData
	{

		/**
		 * An ordered list of unique string ids, each which have a value in `textStrings`.
		 */
		std::set<std::string> stringIds;

		/**
		 * A map of string-id to the RenderableString data each represents.
		 */
		std::unordered_map<std::string, GlyphString> strings;

		bool bHasChanges;

	};
	struct GlyphStringIndices
	{
		std::string stringId;
		uIndex idxDescriptor;
		ui32 idxStartIndex;
		ui32 indexCount;
		ui32 vertexPreCount;
	};
	struct CommittedTextData
	{
		// TODO: Sort the ids according to font id and size (idxDescriptor) so different descriptors don't need to be bound for EVERY string render
		std::vector<GlyphStringIndices> strings;
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

		UncommittedTextData uncommittedData;
		CommittedTextData committedData;

		std::shared_ptr<Memory> memoryTextBuffers;
		graphics::Buffer vertexBuffer;
		graphics::Buffer indexBuffer;

	} mText;

	void updateGlyphVertices(UIString const* updatedString, GlyphString &glyphStr) const;
	uIndex getTextDescriptorIdx(std::string const& fontId, ui8 fontSize) const;

};

NS_END
