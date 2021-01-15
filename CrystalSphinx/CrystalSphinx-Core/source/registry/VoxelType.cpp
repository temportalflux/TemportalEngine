#include "registry/VoxelType.hpp"

#include "asset/BlockType.hpp"

using namespace game;

void VoxelTypeRegistry::registerEntry(BlockTypePath assetPath)
{
	// synchronously load the asset
	// can optimize in the future to async load the asset and actually register once the asset is loaded
	auto asset = assetPath.load(asset::EAssetSerialization::Binary);

	// Register the voxel type, but don't retain the actual voxel type asset in memory
	auto voxelId = asset->uniqueId();
	auto iterFound = this->mEntriesById.find(voxelId);
	if (iterFound != this->mEntriesById.end())
	{
		this->mConflicts.insert(std::make_pair(voxelId, assetPath));
		return;
	}

	// can add id to the registry
	this->mIds.insert(voxelId);
	this->mEntriesById.insert(std::make_pair(voxelId, assetPath));
	this->mMetadataById.insert(std::make_pair(voxelId, LoadedMetadata {
		asset->getIsTranslucent()
	}));

	this->onEntryRegistered.broadcast(voxelId, assetPath);
}

void VoxelTypeRegistry::registerEntries(std::vector<BlockTypePath> const& collection)
{
	for (auto const& item : collection)
	{
		this->registerEntry(item);
	}
}

std::unordered_set<BlockId> const& VoxelTypeRegistry::getIds() const
{
	return this->mIds;
}

std::unordered_map<BlockId, VoxelTypeRegistry::BlockTypePath> const& VoxelTypeRegistry::getEntriesById() const
{
	return this->mEntriesById;
}

bool VoxelTypeRegistry::isTranslucent(BlockId const& id) const { return this->mMetadataById.at(id).bIsTranslucent; }
