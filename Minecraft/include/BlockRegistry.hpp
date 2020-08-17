#pragma once

#include "CoreInclude.hpp"

#include "asset/TypedAssetPath.hpp"

#include "BlockId.hpp"

class StitchedTexture;
FORWARD_DEF(NS_ASSET, class BlockType);
FORWARD_DEF(NS_ASSET, class Texture);
FORWARD_DEF(NS_ASSET, class TextureSampler);
FORWARD_DEF(NS_GRAPHICS, class GameRenderer);
FORWARD_DEF(NS_GRAPHICS, class ImageSampler);

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
		struct TextureSet
		{
			struct Entry
			{
				asset::AssetPath key;
				math::Vector2 offset;
				math::Vector2 size;
			};

			std::weak_ptr<StitchedTexture> atlas;
			std::weak_ptr<graphics::ImageSampler> sampler;
			Entry right, left;
			Entry front, back;
			Entry up, down;
		};

		BlockId id;
		BlockTypePath assetPath;
		TextureSet textureSet;
	};

	BlockRegistry();

	void append(std::vector<BlockTypePath> const& collection);
	void create(std::shared_ptr<graphics::GameRenderer> renderer);
	void destroy();

private:
	std::vector<RegisteredType> mEntries;
	std::unordered_map<asset::AssetPath, uIndex> mEntriesByPath;
	std::unordered_map<BlockId, uIndex> mEntriesById;
	std::unordered_multimap<BlockId, BlockTypePath> mConflicts;

	std::vector<std::shared_ptr<graphics::ImageSampler>> mSamplers;
	std::unordered_map<asset::AssetPath, std::weak_ptr<graphics::ImageSampler>> mSamplerByPath;

	std::vector<std::shared_ptr<StitchedTexture>> mStitchedTextures;

	void addSampler(RegisteredType *entry, SamplerPath samplerPath);
	void addTexturesToStitch(
		RegisteredType *entry,
		TexturePath const &right, TexturePath const &left,
		TexturePath const &front, TexturePath const &back,
		TexturePath const &up, TexturePath const &down
	);
	std::shared_ptr<StitchedTexture> findBestSuitedAtlas(math::Vector2UInt const &entrySize, uSize const count);

};

NS_END
