#include "world/World.hpp"

using namespace world;

void World::loadChunk(math::Vector3Int const &coordinate)
{
	this->mActiveChunks.insert(
		this->mActiveChunks.end(),
		WorldChunk(this->weak_from_this(), coordinate)
	)->load();
}

void World::unloadChunk(math::Vector3Int const &coordinate)
{
	this->unloadChunks({ coordinate });
}

void World::unloadChunks(std::vector<math::Vector3Int> coordinates)
{
	// Active Chunks is relatively small, so its fine to iterate
	this->mActiveChunks.erase(std::remove_if(
		this->mActiveChunks.begin(), this->mActiveChunks.end(),
		[coordinates](auto const& chunk)
		{
		for (auto const& coordToUnload : coordinates)
			if (chunk.coordinate() == coordToUnload) return true;
		return false;
		}
	), this->mActiveChunks.end());
	// TODO: Move the active chunk to a "pending write to disk" operation list
}

void World::markCoordinateDirty(
	world::Coordinate const &global,
	std::optional<BlockMetadata> const& prev,
	std::optional<BlockMetadata> const& next
)
{
	this->markCoordinateWithDirtyNeighbor(global + math::Vector3Int({ -1, 0, 0 }), global);
	this->markCoordinateWithDirtyNeighbor(global + math::Vector3Int({ +1, 0, 0 }), global);
	this->markCoordinateWithDirtyNeighbor(global + math::Vector3Int({ 0, -1, 0 }), global);
	this->markCoordinateWithDirtyNeighbor(global + math::Vector3Int({ 0, +1, 0 }), global);
	this->markCoordinateWithDirtyNeighbor(global + math::Vector3Int({ 0, 0, -1 }), global);
	this->markCoordinateWithDirtyNeighbor(global + math::Vector3Int({ 0, 0, +1 }), global);
	this->mBlockRenderInstances.updateCoordinate(global, prev, next);
}

void World::markCoordinateWithDirtyNeighbor(world::Coordinate const &global, world::Coordinate const &dirtyNeighbor)
{
	auto iter = this->findDirtyNeighborPair(global);
	if (iter == this->mDirtyNeighborCoordinates.end())
	{
		iter = this->mDirtyNeighborCoordinates.insert(
			this->mDirtyNeighborCoordinates.end(),
			std::make_pair(global, std::vector<world::Coordinate>())
		);
	}
	for (auto const& dirtyCoord : iter->second)
	{
		if (dirtyCoord == global) return;
	}
	iter->second.push_back(dirtyNeighbor);
}

World::DirtyNeighborPairList::iterator World::findDirtyNeighborPair(world::Coordinate const &global)
{
	return std::find_if(
		this->mDirtyNeighborCoordinates.begin(),
		this->mDirtyNeighborCoordinates.end(),
		[global](auto const& dirtyPair) { return dirtyPair.first == global; }
	);
}

void World::handleDirtyCoordinates()
{
	// TODO: actually do something here
	this->mDirtyNeighborCoordinates.clear();
}
