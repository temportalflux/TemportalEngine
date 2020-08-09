#pragma once

#include "CoreInclude.hpp"

#include "asset/TypedAssetPath.hpp"

#include "BlockId.hpp"

FORWARD_DEF(NS_ASSET, class BlockType);

FORWARD_DEF(NS_GRAPHICS, class ImageView);
FORWARD_DEF(NS_GRAPHICS, class ImageSampler);
struct StitchedTextureBlock
{
	struct TextureEntry
	{
		math::Vector2 offset;
		math::Vector2 size;
	};

	std::weak_ptr<graphics::ImageView> view;
	std::weak_ptr<graphics::ImageSampler> sampler;
	TextureEntry right, left;
	TextureEntry front, back;
	TextureEntry up, down;
};

NS_GAME

class BlockRegistry
{
	typedef BlockId TKey;
	typedef asset::TypedAssetPath<asset::BlockType> TValue;

public:

	void append(std::vector<TValue> const& collection);

private:
	std::unordered_map<TKey, TValue> mEntries;
	std::unordered_multimap<TKey, TValue> mConflicts;

};

NS_END
