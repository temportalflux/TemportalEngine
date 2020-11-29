#pragma once

#include "CoreInclude.hpp"

#include "Model.hpp"
#include "asset/TypedAssetPath.hpp"
#include "graphics/Buffer.hpp"

#include "BlockId.hpp"

class StitchedTexture;
FORWARD_DEF(NS_ASSET, class BlockType);
FORWARD_DEF(NS_ASSET, class Texture);
FORWARD_DEF(NS_ASSET, class TextureSampler);
FORWARD_DEF(NS_GAME, class VoxelTypeRegistry);
FORWARD_DEF(NS_GRAPHICS, class GameRenderer);
FORWARD_DEF(NS_GRAPHICS, class ImageSampler);
FORWARD_DEF(NS_GRAPHICS, class GraphicsDevice);
FORWARD_DEF(NS_GRAPHICS, class Memory);

NS_GAME

class VoxelModelManager
{
	typedef asset::TypedAssetPath<asset::BlockType> BlockTypePath;
	typedef std::shared_ptr<asset::BlockType> BlockTypeAsset;
	typedef asset::TypedAssetPath<asset::Texture> TexturePath;
	typedef asset::TypedAssetPath<asset::TextureSampler> SamplerPath;

public:

	struct BufferProfile
	{
		graphics::Buffer *vertexBuffer;
		/**
		 * A buffer of indicies to render.
		 * Each value at a given index is the index to a vertex in the vertexBuffer.
		 */
		graphics::Buffer *indexBuffer;
		/**
		 * The index of the first value in the index buffer to render.
		 */
		ui32 indexBufferStartIndex;
		/**
		 * The number of values to render via the index buffer.
		 */
		ui32 indexBufferCount;
		/**
		 * The offset amount to add to the index-buffer-value being rendered.
		 * The index that will be used to loop-up a vertex in the vertexBuffer will be
		 * `lookupIndex = i + indexBufferValueOffset` where `i` is in the range [indexBufferStartIndex, indexBufferStartIndex + indexBufferCount).
		 * This is needed because voxel model indicies are written to the index buffer only relative to that model.
		 * So if a model has 24 verticies (4 verts per cube face), the index buffer would have 36 values, each in the range of [0, 24).
		 * Because indicies are processed as if they are global to the buffer, this offset allows us to say that a given range of indicies
		 * should be processed with additional offset to account for other models in the buffer.
		 */
		ui32 indexBufferValueOffset;
	};

	VoxelModelManager();
	~VoxelModelManager();

	void setSampler(SamplerPath const& samplerPath);
	void loadRegistry(std::shared_ptr<game::VoxelTypeRegistry> registry);
	void loadVoxelTextures(BlockId const& id, BlockTypePath const& assetPath);

	void createTextures(std::shared_ptr<graphics::GraphicsDevice> device, graphics::CommandPool* transientPool);
	void destroyTextures();
	void createModels(std::shared_ptr<graphics::GraphicsDevice> device, graphics::CommandPool* transientPool);
	void destroyModels();

	std::shared_ptr<graphics::ImageSampler> getSampler() const;
	uSize getAtlasCount() const;
	uIndex getAtlasIndex(BlockId const& id) const;
	std::shared_ptr<StitchedTexture> getAtlas(uIndex const idx) const;
	BufferProfile getBufferProfile(BlockId const &blockId);

private:

	struct VoxelTextureEntry
	{
		struct TextureSetHandle
		{
			std::weak_ptr<StitchedTexture> atlas;
			uIndex idxAtlas;

			asset::AssetPath right, left;
			asset::AssetPath front, back;
			asset::AssetPath up, down;

			Model createModel() const;
		};

		TextureSetHandle textureSetHandle;

		Model model;

		/**
		 * The index of the first value in the index buffer to render.
		 */
		ui32 indexBufferStartIndex;

		/**
		 * The number of values to render via the index buffer.
		 */
		ui32 indexCount() const;

		/**
		 * The offset amount to add to the index-buffer-value being rendered.
		 * The index that will be used to loop-up a vertex in the vertexBuffer will be
		 * `lookupIndex = i + indexBufferValueOffset` where `i` is in the range [indexBufferStartIndex, indexBufferStartIndex + indexBufferCount).
		 * This is needed because voxel model indicies are written to the index buffer only relative to that model.
		 * So if a model has 24 verticies (4 verts per cube face), the index buffer would have 36 values, each in the range of [0, 24).
		 * Because indicies are processed as if they are global to the buffer, this offset allows us to say that a given range of indicies
		 * should be processed with additional offset to account for other models in the buffer.
		 */
		ui32 indexBufferValueOffset;

	};

	std::unordered_map<BlockId, VoxelTextureEntry> mEntriesById;

	std::shared_ptr<graphics::ImageSampler> mpSampler;

	std::vector<std::shared_ptr<StitchedTexture>> mStitchedTextures;
	graphics::Buffer mModelVertexBuffer, mModelIndexBuffer;

	void addTexturesToStitch(
		VoxelTextureEntry *entry,
		TexturePath const &right, TexturePath const &left,
		TexturePath const &front, TexturePath const &back,
		TexturePath const &up, TexturePath const &down
	);
	std::optional<uIndex> findBestSuitedAtlas(math::Vector2UInt const &entrySize, uSize const count);
	void createModelBuffers(std::shared_ptr<graphics::GraphicsDevice> device, uSize modelVertexBufferSize, uSize modelIndexBufferSize);
	
};

NS_END
