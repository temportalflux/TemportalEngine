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
		graphics::Buffer *vertexBuffer, *indexBuffer;
		uIndex idxIndiciesStart; // TODO: This is also the amount of which to add to all indicies in the draw call because indicies in the buffer are per-model, and do not account for all indicies in the buffer.
		uSize indexCount;
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
		uSize indexPreCount; // amount of indicies in the index buffer (`VoxelModelManager#mIndexBuffer`) before the indicies for this model
		uSize indexCount() const;
	};

	std::unordered_map<BlockId, VoxelTextureEntry> mEntriesById;

	std::shared_ptr<graphics::ImageSampler> mpSampler;

	std::vector<std::shared_ptr<StitchedTexture>> mStitchedTextures;
	std::shared_ptr<graphics::Memory> mpMemoryModelBuffers;
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
