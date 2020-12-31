#pragma once

#include "CoreInclude.hpp"

#include "Delegate.hpp"
#include "asset/TypedAssetPath.hpp"

#include "BlockId.hpp"

FORWARD_DEF(NS_ASSET, class BlockType);

NS_GAME

class VoxelTypeRegistry
{
	typedef asset::TypedAssetPath<asset::BlockType> BlockTypePath;
	typedef std::shared_ptr<asset::BlockType> BlockTypeAsset;

public:
	
	BroadcastDelegate<void(BlockId const& id, BlockTypePath const& assetPath)> onEntryRegistered;

	void registerEntry(BlockTypePath assetPath);
	void registerEntries(std::vector<BlockTypePath> const& collection);

	std::unordered_set<BlockId> const& getIds() const;
	std::unordered_map<BlockId, BlockTypePath> const& getEntriesById() const;
	bool isTranslucent(BlockId const& id) const;

private:
	struct LoadedMetadata
	{
		bool bIsTranslucent;
	};

	std::unordered_set<BlockId> mIds;
	std::unordered_map<BlockId, BlockTypePath> mEntriesById;
	std::unordered_map<BlockId, LoadedMetadata> mMetadataById;
	std::unordered_multimap<BlockId, BlockTypePath> mConflicts;

};

NS_END
