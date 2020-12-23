#pragma once

#include "CoreInclude.hpp"
#include "asset/TypedAssetPath.hpp"
#include "graphics/Buffer.hpp"
#include "graphics/Descriptor.hpp"
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
		uSize maximumDisplayedCharacters
	);
	~UIRenderer();

	void lock();
	void unlock();
	bool hasChanges() const;

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

	/**
	 * Writes any changes made by `updateGlyphVertices` to the GPU buffer object.
	 */
	void commitToBuffer(graphics::CommandPool* transientPool);

protected:

	void addString(std::shared_ptr<UIString> pStr);
	void removeString(UIString const* pStr);
	void updateString(UIString const* pStr);
	
private:
	std::weak_ptr<graphics::GraphicsDevice> mpDevice;

	math::Vector2UInt mScreenResolution;
	thread::MutexLock mMutex;

	struct FontGlyphVertex
	{
		/**
		 * <x,y> The position of a text/string vertex.
		 * <z, w> The width and edge distance of the character in sdf.
		 */
		math::Vector4 positionAndWidthEdge;
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
		uIndex idxFont;
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
		graphics::DescriptorLayout descriptorLayout;
		// The sampler vulkan uses to read each font atlas
		graphics::ImageSampler sampler;
		// A map of all fonts, their typefaces, and font sizes, including their atlases for each combination
		std::map<std::string, uIndex> fontIds;
		std::vector<graphics::Font> fonts;
		// the number of faces across all fonts
		ui8 fontFaceCount;

		UncommittedTextData uncommittedData;
		CommittedTextData committedData;

		graphics::Buffer vertexBuffer;
		graphics::Buffer indexBuffer;

	} mText;

	void updateGlyphString(GlyphString &glyphStr);
	void updateGlyphVertices(UIString const* updatedString, GlyphString &glyphStr) const;

};

NS_END
