#include "render/VoxelModelManager.hpp"

#include "StitchedTexture.hpp"
#include "asset/BlockType.hpp"
#include "asset/Texture.hpp"
#include "asset/TextureSampler.hpp"
#include "graphics/ImageSampler.hpp"
#include "graphics/GraphicsDevice.hpp"
#include "model/CubeModelLoader.hpp"
#include "registry/VoxelType.hpp"

using namespace game;

VoxelModelManager::VoxelModelManager()
{
	this->mStitchedTextures.push_back(std::make_shared<StitchedTexture, math::Vector2UInt, math::Vector2UInt, math::Vector2UInt>(
		{ 256, 256 }, { 2048, 2048 }, { 16, 16 }
	));
}

VoxelModelManager::~VoxelModelManager()
{
	destroyModels();
	destroyTextures();
}

void VoxelModelManager::setSampler(SamplerPath const& samplerPath)
{
	auto samplerAsset = samplerPath.load(asset::EAssetSerialization::Binary);
	this->mpSampler = std::make_shared<graphics::ImageSampler>();
	this->mpSampler
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
}

std::shared_ptr<graphics::ImageSampler> VoxelModelManager::getSampler() const
{
	return this->mpSampler;
}

void VoxelModelManager::loadRegistry(std::shared_ptr<game::VoxelTypeRegistry> registry)
{
	for (auto const& idPath : registry->getEntriesById())
	{
		this->loadVoxelTextures(idPath.first, idPath.second);
	}
}

void VoxelModelManager::loadVoxelTextures(BlockId const& id, BlockTypePath const& assetPath)
{
	auto asset = assetPath.load(asset::EAssetSerialization::Binary);
	auto textureSet = asset->textureSet();

	auto iterHandle = this->mEntriesById.insert(std::make_pair(id, VoxelTextureEntry()));
	auto& entry = iterHandle.first->second;

	//this->addSampler(&entry, textureSet.sampler);
	this->addTexturesToStitch(&entry, textureSet.right, textureSet.left, textureSet.front, textureSet.back, textureSet.up, textureSet.down);
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

void VoxelModelManager::addTexturesToStitch(
	VoxelTextureEntry *entry,
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

	auto idxAtlas = this->findBestSuitedAtlas(entrySize, textures.size());
	assert(idxAtlas); // TODO: If we don't have a stitched texture, make one
	auto atlas = this->mStitchedTextures[*idxAtlas];
	if (!atlas->addTextures(textures))
	{
		// ERROR
		assert(false);
	}

	entry->textureSetHandle.atlas = atlas;
	entry->textureSetHandle.idxAtlas = *idxAtlas;
	entry->textureSetHandle.right = rightPath.path();
	entry->textureSetHandle.left = leftPath.path();
	entry->textureSetHandle.front = frontPath.path();
	entry->textureSetHandle.back = backPath.path();
	entry->textureSetHandle.up = upPath.path();
	entry->textureSetHandle.down = downPath.path();
}

std::optional<uIndex> VoxelModelManager::findBestSuitedAtlas(math::Vector2UInt const &entrySize, uSize const count)
{
	for (uIndex i = 0; i < this->mStitchedTextures.size(); ++i)
	{
		auto const& atlas = this->mStitchedTextures[i];
		if (atlas->getSizePerEntry() == entrySize && atlas->canAdd(count)) return i;
	}
	return std::nullopt;
}

void VoxelModelManager::createTextures(std::shared_ptr<graphics::GraphicsDevice> device, graphics::CommandPool* transientPool)
{
	this->mpSampler->setDevice(device);
	this->mpSampler->create();

	for (auto& atlas : this->mStitchedTextures)
	{
		atlas->finalize(device, transientPool);
	}
}

void VoxelModelManager::destroyTextures()
{
	this->mStitchedTextures.clear();
	this->mpSampler.reset();
}

void VoxelModelManager::createModels(std::shared_ptr<graphics::GraphicsDevice> device, graphics::CommandPool* transientPool)
{
	uSize modelVertexBufferSize = 0;
	uSize modelIndexBufferSize = 0;
	for (auto& entry : this->mEntriesById)
	{
		entry.second.model = entry.second.textureSetHandle.createModel();
		modelVertexBufferSize += entry.second.model.getVertexBufferSize();
		modelIndexBufferSize += entry.second.model.getIndexBufferSize();
	}

	this->createModelBuffers(device, modelVertexBufferSize, modelIndexBufferSize);

	auto vertices = std::vector<Model::Vertex>();
	auto indices = std::vector<ui16>();
	for (auto& entry : this->mEntriesById)
	{
		auto const& modelVertices = entry.second.model.verticies();
		auto const& modelIndices = entry.second.model.indicies();
		entry.second.indexBufferStartIndex = (ui32)indices.size();
		entry.second.indexBufferValueOffset = (ui32)vertices.size();
		std::copy(modelVertices.begin(), modelVertices.end(), std::back_inserter(vertices));
		std::copy(modelIndices.begin(), modelIndices.end(), std::back_inserter(indices));
	}
	// TODO: These can be done in one operation, and we don't need to wait for the graphics device to be done (nothing relies on this process except starting rendering)
	this->mModelVertexBuffer.writeBuffer(transientPool, 0, vertices);
	this->mModelIndexBuffer.writeBuffer(transientPool, 0, indices);
}

Model VoxelModelManager::VoxelTextureEntry::TextureSetHandle::createModel() const
{
	auto loader = CubeModelLoader();
	auto atlas = this->atlas.lock();

	{
		auto atlasDatum = atlas->getStitchedTexture(this->right);
		assert(atlasDatum);
		loader.pushRight(atlasDatum->offset, atlasDatum->size);
	}
	{
		auto atlasDatum = atlas->getStitchedTexture(this->left);
		assert(atlasDatum);
		loader.pushLeft(atlasDatum->offset, atlasDatum->size);
	}
	{
		auto atlasDatum = atlas->getStitchedTexture(this->front);
		assert(atlasDatum);
		loader.pushFront(atlasDatum->offset, atlasDatum->size);
	}
	{
		auto atlasDatum = atlas->getStitchedTexture(this->back);
		assert(atlasDatum);
		loader.pushBack(atlasDatum->offset, atlasDatum->size);
	}
	{
		auto atlasDatum = atlas->getStitchedTexture(this->up);
		assert(atlasDatum);
		loader.pushUp(atlasDatum->offset, atlasDatum->size);
	}
	{
		auto atlasDatum = atlas->getStitchedTexture(this->down);
		assert(atlasDatum);
		loader.pushDown(atlasDatum->offset, atlasDatum->size);
	}

	return loader.get();
}

void VoxelModelManager::createModelBuffers(std::shared_ptr<graphics::GraphicsDevice> device, uSize modelVertexBufferSize, uSize modelIndexBufferSize)
{
	this->mModelVertexBuffer.setDevice(device);
	this->mModelVertexBuffer
		.setUsage(vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer, graphics::MemoryUsage::eGPUOnly)
		.setSize(modelVertexBufferSize)
		.create();

	this->mModelIndexBuffer.setDevice(device);
	this->mModelIndexBuffer
		.setUsage(vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer, graphics::MemoryUsage::eGPUOnly)
		.setSize(modelIndexBufferSize)
		.create();
}

void VoxelModelManager::destroyModels()
{
	this->mModelVertexBuffer.destroy();
	this->mModelIndexBuffer.destroy();
}

ui32 VoxelModelManager::VoxelTextureEntry::indexCount() const
{
	return (ui32)this->model.indicies().size();
}

uSize VoxelModelManager::getAtlasCount() const
{
	return this->mStitchedTextures.size();
}

uIndex VoxelModelManager::getAtlasIndex(BlockId const& id) const
{
	auto const iterIdxEntry = this->mEntriesById.find(id);
	assert(iterIdxEntry != this->mEntriesById.end());
	return iterIdxEntry->second.textureSetHandle.idxAtlas;
}

std::shared_ptr<StitchedTexture> VoxelModelManager::getAtlas(uIndex const idx) const
{
	return this->mStitchedTextures[idx];
}

VoxelModelManager::BufferProfile VoxelModelManager::getBufferProfile(BlockId const &blockId)
{
	OPTICK_EVENT();
	auto const iterIdxEntry = this->mEntriesById.find(blockId);
	assert(iterIdxEntry != this->mEntriesById.end());
	auto const& entry = iterIdxEntry->second;
	return {
		&this->mModelVertexBuffer, &this->mModelIndexBuffer,
		entry.indexBufferStartIndex, entry.indexCount(), entry.indexBufferValueOffset
	};
}
