#include "render/voxel/VoxelModelManager.hpp"

#include "Engine.hpp"
#include "logging/Logger.hpp"

#include "StitchedTexture.hpp"
#include "asset/BlockType.hpp"
#include "asset/Texture.hpp"
#include "asset/TextureSampler.hpp"
#include "graphics/assetHelpers.hpp"
#include "graphics/ImageSampler.hpp"
#include "graphics/GraphicsDevice.hpp"
#include "registry/VoxelType.hpp"
#include "resource/ResourceManager.hpp"

using namespace game;

static logging::Logger VoxelModelLog = DeclareLog("VoxelModelManager", LOG_INFO);

VoxelModelManager::VoxelModelManager(std::weak_ptr<graphics::GraphicsDevice> device, graphics::CommandPool* transientPool)
	: mpDevice(device), mpTransientPool(transientPool)
{
}

VoxelModelManager::~VoxelModelManager()
{
	destroyModels();
	destroyTextures();
	this->mpSampler.reset();
}

void VoxelModelManager::setSampler(SamplerPath const& samplerPath)
{
	this->mpSampler = std::make_shared<graphics::ImageSampler>();
	this->mpSampler->setDevice(this->mpDevice);
	graphics::populateSampler(this->mpSampler.get(), samplerPath);
	this->mpSampler->create();
}

std::shared_ptr<graphics::ImageSampler> VoxelModelManager::getSampler() const
{
	return this->mpSampler;
}

void VoxelModelManager::loadRegistry(std::shared_ptr<game::VoxelTypeRegistry> registry)
{
	for (auto const& idPath : registry->getEntriesById())
	{
		auto asset = idPath.second.load(asset::EAssetSerialization::Binary);
		auto iterHandle = this->mEntriesById.insert(std::make_pair(idPath.first, VoxelTextureEntry()));
		auto& entry = iterHandle.first->second;
		
		entry.textureSetHandle.right = asset->textureSet().right;
		entry.textureSetHandle.left = asset->textureSet().left;
		entry.textureSetHandle.front = asset->textureSet().front;
		entry.textureSetHandle.back = asset->textureSet().back;
		entry.textureSetHandle.up = asset->textureSet().up;
		entry.textureSetHandle.down = asset->textureSet().down;
		entry.textureSetHandle.uniqueIds = {
			entry.textureSetHandle.right,
			entry.textureSetHandle.left,
			entry.textureSetHandle.front,
			entry.textureSetHandle.back,
			entry.textureSetHandle.up,
			entry.textureSetHandle.down,
		};
	}
}

std::function<void(resource::PackManager*)> VoxelModelManager::onTexturesLoadedEvent()
{
	return std::bind(&VoxelModelManager::onTexturesLoaded, this, std::placeholders::_1);
}

void VoxelModelManager::onTexturesLoaded(resource::PackManager *packManager)
{
	destroyModels();
	destroyTextures();

	auto entries = packManager->getTexturesOfType("voxel");
	auto packEntriesByTextureId = std::unordered_map<std::string, resource::PackManager::PackEntry>();
	for (auto const& entry : entries)
	{
		packEntriesByTextureId.insert(std::make_pair(entry.textureId(), entry));
	}

	for (auto& voxelEntry : this->mEntriesById)
	{
		// Ensure that all the textures for a given voxel type have the same atlas
		bool bCanPackVoxel = true;
		std::optional<math::Vector2UInt> prevSize = std::nullopt;
		{
			std::optional<std::string> prev = std::nullopt;
			for (auto const& shortId : voxelEntry.second.textureSetHandle.uniqueIds)
			{
				auto const texId = "voxel:" + shortId;
				auto nextIter = packEntriesByTextureId.find(texId);
				if (!prev)
				{
					prev = texId;
					prevSize = packManager->getTextureData(nextIter->second).size;
					continue;
				}

				auto prevIter = packEntriesByTextureId.find(*prev);
				if (nextIter == packEntriesByTextureId.end())
				{
					// ERROR texture not found
					VoxelModelLog.log(LOG_ERR, "Failed to find texture id '%s' for voxel '%s'", texId, voxelEntry.first);
					bCanPackVoxel = false;
					continue;
				}

				auto nextSize = packManager->getTextureData(nextIter->second).size;
				if (nextSize != *prevSize)
				{
					// ERROR sizes dont match
					VoxelModelLog.log(LOG_ERR, "Voxel '%s' textures '%s' and '%s' do not have the same size.", voxelEntry.first, *prev, texId);
					bCanPackVoxel = false;
					continue;
				}
			}
		}
		if (!bCanPackVoxel) continue;

		uSize textureCount = voxelEntry.second.textureSetHandle.uniqueIds.size();
		auto idxAtlas = this->findBestSuitedAtlas(*prevSize, textureCount);
		// If we don't have a stitched texture, make one
		if (!idxAtlas)
		{
			idxAtlas = this->createAtlas(*prevSize);
		}

		auto textureData = std::vector<std::pair<std::string, std::vector<ui8>>>();
		for (auto const& shortId : voxelEntry.second.textureSetHandle.uniqueIds)
		{
			auto const texId = "voxel:" + shortId;
			auto const& packEntry = packEntriesByTextureId.find(texId)->second;
			auto const& texData = packManager->getTextureData(packEntry);
			textureData.push_back(std::make_pair(shortId, texData.pixels));
		}

		auto atlas = this->mStitchedTextures[*idxAtlas];
		if (!atlas->addTextures(textureData))
		{
			VoxelModelLog.log(LOG_ERR, "Failed to load %i textures into a texture atlas", textureCount);
		}
		voxelEntry.second.textureSetHandle.idxAtlas = *idxAtlas;
	}

	createTextures();
	createModels();

	this->OnAtlasesCreatedEvent.broadcast();
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

uIndex VoxelModelManager::createAtlas(math::Vector2UInt const& entrySize)
{
	uIndex idx = this->mStitchedTextures.size();
	this->mStitchedTextures.push_back(std::make_shared<StitchedTexture>(
		math::Vector2UInt{ 256, 256 }, math::Vector2UInt{ 2048, 2048 }, entrySize
	));
	return idx;
}

void VoxelModelManager::createTextures()
{
	for (auto& atlas : this->mStitchedTextures)
	{
		atlas->finalize(this->mpDevice.lock(), this->mpTransientPool);
	}
}

void VoxelModelManager::destroyTextures()
{
	this->mStitchedTextures.clear();
}

void VoxelModelManager::createModels()
{
	uSize modelVertexBufferSize = 0;
	uSize modelIndexBufferSize = 0;
	for (auto& entry : this->mEntriesById)
	{
		this->createModel(entry.second.textureSetHandle, entry.second.model);
		modelVertexBufferSize += entry.second.model.getVertexBufferSize();
		modelIndexBufferSize += entry.second.model.getIndexBufferSize();
	}

	this->createModelBuffers(this->mpDevice.lock(), modelVertexBufferSize, modelIndexBufferSize);

	auto vertices = std::vector<ModelVoxelVertex>();
	auto indices = std::vector<ui32>();
	for (auto& entry : this->mEntriesById)
	{
		auto const& modelVertices = entry.second.model.vertices();
		auto const& modelIndices = entry.second.model.indices();
		entry.second.indexBufferStartIndex = (ui32)indices.size();
		entry.second.indexBufferValueOffset = (ui32)vertices.size();
		std::copy(modelVertices.begin(), modelVertices.end(), std::back_inserter(vertices));
		std::copy(modelIndices.begin(), modelIndices.end(), std::back_inserter(indices));
	}
	// TODO: These can be done in one operation, and we don't need to wait for the graphics device to be done (nothing relies on this process except starting rendering)
	this->mModelVertexBuffer.writeBuffer(this->mpTransientPool, 0, vertices);
	this->mModelIndexBuffer.writeBuffer(this->mpTransientPool, 0, indices);
}

void VoxelModelManager::createModel(VoxelTextureEntry::TextureSetHandle const& handle, ModelVoxel &out) const
{
	auto atlas = this->mStitchedTextures[handle.idxAtlas];

	{
		auto atlasDatum = atlas->getStitchedTexture(handle.right);
		assert(atlasDatum);
		out.pushRight(atlasDatum->offset, atlasDatum->size);
	}
	{
		auto atlasDatum = atlas->getStitchedTexture(handle.left);
		assert(atlasDatum);
		out.pushLeft(atlasDatum->offset, atlasDatum->size);
	}
	{
		auto atlasDatum = atlas->getStitchedTexture(handle.front);
		assert(atlasDatum);
		out.pushFront(atlasDatum->offset, atlasDatum->size);
	}
	{
		auto atlasDatum = atlas->getStitchedTexture(handle.back);
		assert(atlasDatum);
		out.pushBack(atlasDatum->offset, atlasDatum->size);
	}
	{
		auto atlasDatum = atlas->getStitchedTexture(handle.up);
		assert(atlasDatum);
		out.pushUp(atlasDatum->offset, atlasDatum->size);
	}
	{
		auto atlasDatum = atlas->getStitchedTexture(handle.down);
		assert(atlasDatum);
		out.pushDown(atlasDatum->offset, atlasDatum->size);
	}
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
	return (ui32)this->model.indices().size();
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
