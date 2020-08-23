#include "BlockRegistry.hpp"

#include "StitchedTexture.hpp"
#include "asset/BlockType.hpp"
#include "asset/Texture.hpp"
#include "asset/TextureSampler.hpp"
#include "graphics/ImageSampler.hpp"
#include "graphics/GameRenderer.hpp"
#include "model/CubeModelLoader.hpp"

using namespace game;

BlockRegistry::BlockRegistry()
{
	this->mStitchedTextures.push_back(std::make_shared<StitchedTexture, math::Vector2UInt, math::Vector2UInt, math::Vector2UInt>(
		{ 256, 256 }, { 2048, 2048 }, { 16, 16 }
	));
}

void BlockRegistry::append(std::vector<BlockTypePath> const& collection)
{
	auto assets = std::vector<std::pair<BlockTypeAsset, BlockTypePath>>();
	// synchronously load each asset
	for (auto const& item : collection)
	{
		assets.push_back(std::make_pair(item.load(asset::EAssetSerialization::Binary), item));
	}
	// Register the block types, but don't retain the actual block type in memory
	for (auto const& valuePair : assets)
	{
		auto blockId = valuePair.first->uniqueId();
		auto iterFound = this->mEntriesById.find(blockId);
		if (iterFound != this->mEntriesById.end())
		{
			this->mConflicts.insert(std::make_pair(blockId, valuePair.second));
			continue;
		}

		// can add id to the registry
		auto iter = this->mEntries.insert(this->mEntries.end(), { blockId, valuePair.second });
		uIndex idx = std::distance(this->mEntries.begin(), iter);
		this->mEntriesById.insert(std::make_pair(blockId, idx));
		this->mEntriesByPath.insert(std::make_pair(valuePair.second.path(), idx));

		auto textureSet = valuePair.first->textureSet();
		this->addSampler(&this->mEntries[idx], textureSet.sampler);
		this->addTexturesToStitch(&this->mEntries[idx], textureSet.right, textureSet.left, textureSet.front, textureSet.back, textureSet.up, textureSet.down);
	}
	// assets released at end of function
}

void BlockRegistry::addSampler(RegisteredType *entry, SamplerPath samplerPath)
{
	auto iterSampler = this->mSamplerByPath.find(samplerPath.path());
	if (iterSampler != this->mSamplerByPath.end())
	{
		entry->textureSet.sampler = iterSampler->second;
		return;
	}

	// Load the asset
	auto samplerAsset = samplerPath.load(asset::EAssetSerialization::Binary);
	
	// Convert into the graphics sampler
	auto sampler = *this->mSamplers.insert(this->mSamplers.end(), std::make_shared<graphics::ImageSampler>());
	sampler
		->setFilter(
			samplerAsset->getFilterModeMagnified(),
			samplerAsset->getFilterModeMinified()
		)
		.setAddressMode(samplerAsset->getAddressModes())
		.setAnistropy(samplerAsset->getAnisotropy())
		.setBorderColor(samplerAsset->getBorderColor())
		.setNormalizeCoordinates(samplerAsset->areCoordinatesNormalized())
		.setCompare(samplerAsset->getCompareOperation())
		.setMipLOD(
			samplerAsset->getLodMode(),
			samplerAsset->getLodBias(), samplerAsset->getLodRange()
		);
	entry->textureSet.sampler = sampler;
	this->mSamplerByPath.insert(std::make_pair(samplerPath.path(), entry->textureSet.sampler));
}

std::vector<asset::TypedAssetPath<asset::Texture>> eliminateDuplicatePaths(std::vector<asset::TypedAssetPath<asset::Texture>> paths)
{
	std::unordered_set<asset::AssetPath> filter;
	paths.erase(std::remove_if(
		paths.begin(), paths.end(),
		[&filter](auto const &entry)
		{
			return !filter.insert(entry.path()).second;
		}
	), paths.end());
	return paths;
}

void BlockRegistry::addTexturesToStitch(
	RegisteredType *entry,
	TexturePath const &rightPath, TexturePath const &leftPath,
	TexturePath const &frontPath, TexturePath const &backPath,
	TexturePath const &upPath, TexturePath const &downPath
)
{
	auto texturePaths = eliminateDuplicatePaths({
		rightPath, leftPath, frontPath, backPath, upPath, downPath 
	});

	// Load all the textures
	std::vector<std::pair<asset::AssetPath, std::shared_ptr<asset::Texture>>> textures;
	math::Vector2UInt entrySize;
	for (uIndex i = 0; i < texturePaths.size(); ++i)
	{
		textures.push_back(std::make_pair(texturePaths[i].path(), texturePaths[i].load(asset::EAssetSerialization::Binary)));
		if (i == 0) entrySize = textures[i].second->getSourceSize();
		else assert(textures[i].second->getSourceSize() == entrySize);
	}

	auto atlas = this->findBestSuitedAtlas(entrySize, textures.size());
	assert(atlas); // TODO: If we don't have a stitched texture, make one
	if (!atlas->addTextures(textures))
	{
		// ERROR
		assert(false);
	}

	entry->textureSet.atlas = atlas;
	entry->textureSet.right.key = rightPath.path();
	entry->textureSet.left.key = leftPath.path();
	entry->textureSet.front.key = frontPath.path();
	entry->textureSet.back.key = backPath.path();
	entry->textureSet.up.key = upPath.path();
	entry->textureSet.down.key = downPath.path();
}

std::shared_ptr<StitchedTexture> BlockRegistry::findBestSuitedAtlas(math::Vector2UInt const &entrySize, uSize const count)
{
	for (auto& atlas : this->mStitchedTextures)
	{
		if (atlas->getSizePerEntry() == entrySize && atlas->canAdd(count)) return atlas;
	}
	return nullptr;
}

void fetchTextureData(std::shared_ptr<StitchedTexture> atlas, BlockRegistry::RegisteredType::TextureSet::Entry &datum)
{
	auto atlasDatum = atlas->getStitchedTexture(datum.key);
	if (!atlasDatum) return;
	datum.offset = atlasDatum->offset;
	datum.size = atlasDatum->size;
}

void fetchTextureData(BlockRegistry::RegisteredType::TextureSet &textureSet)
{
	auto atlas = textureSet.atlas.lock();
	fetchTextureData(atlas, textureSet.right);
	fetchTextureData(atlas, textureSet.left);
	fetchTextureData(atlas, textureSet.front);
	fetchTextureData(atlas, textureSet.back);
	fetchTextureData(atlas, textureSet.up);
	fetchTextureData(atlas, textureSet.down);
}

void createModel(BlockRegistry::RegisteredType &entry)
{
	CubeModelLoader(&entry.model)
		.pushRight(entry.textureSet.right.offset, entry.textureSet.right.size)
		.pushLeft(entry.textureSet.left.offset, entry.textureSet.left.size)
		.pushFront(entry.textureSet.front.offset, entry.textureSet.front.size)
		.pushBack(entry.textureSet.back.offset, entry.textureSet.back.size)
		.pushUp(entry.textureSet.up.offset, entry.textureSet.up.size)
		.pushDown(entry.textureSet.down.offset, entry.textureSet.down.size);
}

void BlockRegistry::create(std::shared_ptr<graphics::GameRenderer> renderer)
{
	for (auto& sampler : this->mSamplers)
	{
		sampler->setDevice(renderer->getDevice());
		sampler->create();
	}
	for (auto& atlas : this->mStitchedTextures)
	{
		atlas->finalize(renderer);
	}
	for (auto& entry : this->mEntries)
	{
		fetchTextureData(entry.textureSet);
		createModel(entry);
	}
}

void BlockRegistry::destroy()
{
	this->mEntriesByPath.clear();
	this->mEntriesById.clear();
	this->mEntries.clear();

	this->mSamplerByPath.clear();
	this->mSamplers.clear();

	this->mStitchedTextures.clear();
}
