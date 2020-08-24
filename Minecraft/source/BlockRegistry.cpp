#include "BlockRegistry.hpp"

#include "StitchedTexture.hpp"
#include "asset/BlockType.hpp"
#include "asset/Texture.hpp"
#include "asset/TextureSampler.hpp"
#include "graphics/ImageSampler.hpp"
#include "graphics/GameRenderer.hpp"
#include "graphics/GraphicsDevice.hpp"
#include "graphics/Memory.hpp"
#include "model/CubeModelLoader.hpp"

using namespace game;

struct TextureSet
{
	struct Entry
	{
		math::Vector2 offset;
		math::Vector2 size;
	};

	Entry right, left;
	Entry front, back;
	Entry up, down;
};

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
		entry->textureSetHandle.sampler = iterSampler->second;
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
	entry->textureSetHandle.sampler = sampler;
	this->mSamplerByPath.insert(std::make_pair(samplerPath.path(), entry->textureSetHandle.sampler));
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

	entry->textureSetHandle.atlas = atlas;
	entry->textureSetHandle.right = rightPath.path();
	entry->textureSetHandle.left = leftPath.path();
	entry->textureSetHandle.front = frontPath.path();
	entry->textureSetHandle.back = backPath.path();
	entry->textureSetHandle.up = upPath.path();
	entry->textureSetHandle.down = downPath.path();
}

std::shared_ptr<StitchedTexture> BlockRegistry::findBestSuitedAtlas(math::Vector2UInt const &entrySize, uSize const count)
{
	for (auto& atlas : this->mStitchedTextures)
	{
		if (atlas->getSizePerEntry() == entrySize && atlas->canAdd(count)) return atlas;
	}
	return nullptr;
}

Model createModel(BlockRegistry::RegisteredType::TextureSetHandle &textureSet)
{
	auto loader = CubeModelLoader();
	auto atlas = textureSet.atlas.lock();

	{
		auto atlasDatum = atlas->getStitchedTexture(textureSet.right);
		assert(atlasDatum);
		loader.pushRight(atlasDatum->offset, atlasDatum->size);
	}
	{
		auto atlasDatum = atlas->getStitchedTexture(textureSet.left);
		assert(atlasDatum);
		loader.pushLeft(atlasDatum->offset, atlasDatum->size);
	}
	{
		auto atlasDatum = atlas->getStitchedTexture(textureSet.front);
		assert(atlasDatum);
		loader.pushFront(atlasDatum->offset, atlasDatum->size);
	}
	{
		auto atlasDatum = atlas->getStitchedTexture(textureSet.back);
		assert(atlasDatum);
		loader.pushBack(atlasDatum->offset, atlasDatum->size);
	}
	{
		auto atlasDatum = atlas->getStitchedTexture(textureSet.up);
		assert(atlasDatum);
		loader.pushUp(atlasDatum->offset, atlasDatum->size);
	}
	{
		auto atlasDatum = atlas->getStitchedTexture(textureSet.down);
		assert(atlasDatum);
		loader.pushDown(atlasDatum->offset, atlasDatum->size);
	}

	return loader.get();
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

	uSize modelVertexBufferSize = 0;
	uSize modelIndexBufferSize = 0;
	for (auto& entry : this->mEntries)
	{
		entry.model = createModel(entry.textureSetHandle);
		modelVertexBufferSize += entry.model.getVertexBufferSize();
		modelIndexBufferSize += entry.model.getIndexBufferSize();
	}

	this->createModelBuffers(renderer->getDevice(), modelVertexBufferSize, modelIndexBufferSize);

	auto vertices = std::vector<Model::Vertex>();
	auto indices = std::vector<ui16>();
	for (auto& entry : this->mEntries)
	{
		auto const& modelVertices = entry.model.verticies();
		auto const& modelIndices = entry.model.indicies();
		entry.indexPreCount = indices.size();
		std::copy(modelVertices.begin(), modelVertices.end(), std::back_inserter(vertices));
		std::copy(modelIndices.begin(), modelIndices.end(), std::back_inserter(indices));
	}
	// TODO: These can be done in one operation, and we dont need to wait for the graphics device to be done (nothing relies on this process except starting rendering)
	this->mVertexBuffer.writeBuffer(&renderer->getTransientPool(), 0, vertices);
	this->mIndexBuffer.writeBuffer(&renderer->getTransientPool(), 0, indices);
}

void BlockRegistry::createModelBuffers(std::shared_ptr<graphics::GraphicsDevice> device, uSize modelVertexBufferSize, uSize modelIndexBufferSize)
{
	this->mpMemoryModelBuffers = std::make_shared<graphics::Memory>();
	this->mpMemoryModelBuffers->setDevice(device);
	this->mpMemoryModelBuffers->setFlags(vk::MemoryPropertyFlagBits::eDeviceLocal);

	this->mVertexBuffer.setDevice(device);
	this->mVertexBuffer
		.setUsage(vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer)
		.setSize(modelVertexBufferSize)
		.create();
	this->mVertexBuffer.configureSlot(this->mpMemoryModelBuffers);

	this->mIndexBuffer.setDevice(device);
	this->mIndexBuffer
		.setUsage(vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer)
		.setSize(modelIndexBufferSize)
		.create();
	this->mIndexBuffer.configureSlot(this->mpMemoryModelBuffers);

	this->mpMemoryModelBuffers->create();

	this->mVertexBuffer.bindMemory();
	this->mIndexBuffer.bindMemory();
}

BlockRegistry::BufferProfile BlockRegistry::getBufferProfile(BlockId const &blockId)
{
	auto const iterIdxEntry = this->mEntriesById.find(blockId);
	assert(iterIdxEntry != this->mEntriesById.end());
	auto const& entry = this->mEntries[iterIdxEntry->second];
	return {
		&this->mVertexBuffer, &this->mIndexBuffer, entry.indexPreCount
	};
}

void BlockRegistry::destroy()
{
	this->mEntriesByPath.clear();
	this->mEntriesById.clear();
	this->mEntries.clear();

	this->mSamplerByPath.clear();
	this->mSamplers.clear();

	this->mStitchedTextures.clear();
	this->mVertexBuffer.destroy();
	this->mIndexBuffer.destroy();
	this->mpMemoryModelBuffers.reset();
}
