#include "BlockRegistry.hpp"

#include "asset/BlockType.hpp"

using namespace game;

void BlockRegistry::append(std::vector<TValue> const& collection)
{
	auto assets = std::vector<std::pair<std::shared_ptr<asset::BlockType>, TValue>>();
	// synchronously load each asset
	for (auto const& item : collection)
	{
		assets.push_back(std::make_pair(item.load(asset::EAssetSerialization::Binary), item));
	}
	// Register the block types, but don't retain the actual block type in memory
	for (auto const& valuePair : assets)
	{
		auto blockId = valuePair.first->uniqueId();
		auto idToTypedPath = std::make_pair(blockId, valuePair.second);
		auto iterFound = this->mEntries.find(blockId);
		if (iterFound == this->mEntries.end())
		{
			// can add id to the registry
			this->mEntries.insert(idToTypedPath);
		}
		else
		{
			this->mConflicts.insert(idToTypedPath);
		}
	}
	// assets released at end of function
}
