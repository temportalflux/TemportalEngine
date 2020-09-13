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
FORWARD_DEF(NS_GRAPHICS, class GameRenderer);
FORWARD_DEF(NS_GRAPHICS, class ImageSampler);
FORWARD_DEF(NS_GRAPHICS, class GraphicsDevice);
FORWARD_DEF(NS_GRAPHICS, class Memory);

NS_GAME

class BlockRegistry
{
	typedef asset::TypedAssetPath<asset::BlockType> BlockTypePath;
	typedef std::shared_ptr<asset::BlockType> BlockTypeAsset;
	typedef asset::TypedAssetPath<asset::Texture> TexturePath;
	typedef asset::TypedAssetPath<asset::TextureSampler> SamplerPath;

public:

	struct RegisteredType
	{
		struct TextureSetHandle
		{
			std::weak_ptr<StitchedTexture> atlas;
			std::weak_ptr<graphics::ImageSampler> sampler;
			asset::AssetPath right, left;
			asset::AssetPath front, back;
			asset::AssetPath up, down;
		};

		BlockId id;
		BlockTypePath assetPath;
		TextureSetHandle textureSetHandle;
		Model model;
		uSize indexPreCount; // amount of indicies in the index buffer (`BlockRegistry#mIndexBuffer`) before the indicies for this model
	};

	typedef std::vector<RegisteredType> RegisteredTypeList;
	typedef RegisteredTypeList::const_iterator IterEntryItem;

	struct BufferProfile
	{
		graphics::Buffer *vertexBuffer, *indexBuffer;
		uIndex idxIndiciesStart; // TODO: This is also the amount of which to add to all indicies in the draw call because indicies in the buffer are per-model, and do not account for all indicies in the buffer.
	};

	BlockRegistry();

	void append(std::vector<BlockTypePath> const& collection);
	void create(std::shared_ptr<graphics::GameRenderer> renderer);
	void destroy();

	IterEntryItem begin() const;
	IterEntryItem end() const;

private:
	RegisteredTypeList mEntries;
	std::unordered_map<asset::AssetPath, uIndex> mEntriesByPath;
	std::unordered_map<BlockId, uIndex> mEntriesById;
	std::unordered_multimap<BlockId, BlockTypePath> mConflicts;

	std::vector<std::shared_ptr<graphics::ImageSampler>> mSamplers;
	std::unordered_map<asset::AssetPath, std::weak_ptr<graphics::ImageSampler>> mSamplerByPath;

	std::vector<std::shared_ptr<StitchedTexture>> mStitchedTextures;
	std::shared_ptr<graphics::Memory> mpMemoryModelBuffers;
	graphics::Buffer mVertexBuffer, mIndexBuffer;

	void addSampler(RegisteredType *entry, SamplerPath samplerPath);
	void addTexturesToStitch(
		RegisteredType *entry,
		TexturePath const &right, TexturePath const &left,
		TexturePath const &front, TexturePath const &back,
		TexturePath const &up, TexturePath const &down
	);
	std::shared_ptr<StitchedTexture> findBestSuitedAtlas(math::Vector2UInt const &entrySize, uSize const count);
	void createModelBuffers(std::shared_ptr<graphics::GraphicsDevice> device, uSize modelVertexBufferSize, uSize modelIndexBufferSize);
	BufferProfile getBufferProfile(BlockId const &blockId);

};

NS_END
