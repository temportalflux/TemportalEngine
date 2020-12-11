#pragma once

#include "CoreInclude.hpp"

#include "Delegate.hpp"

#include "world/Chunk.hpp"
#include "world/WorldCoordinate.hpp"
#include "world/Events.hpp"

NS_WORLD

class World : public std::enable_shared_from_this<World>
{
	friend class WorldChunk;

public:

	TOnChunkLoadingEvent OnLoadingChunk, OnUnloadingChunk;
	TOnVoxelsChangedEvent OnVoxelsChanged;

	void loadChunk(math::Vector3Int const &coordinate);
	void reloadChunk(math::Vector3Int const &coordinate);
	void unloadChunk(math::Vector3Int const &coordinate);
	void unloadChunks(std::vector<math::Vector3Int> coordinates);

	void markCoordinateDirty(
		world::Coordinate const &global,
		std::optional<BlockMetadata> const& prev,
		std::optional<BlockMetadata> const& next
	);
	void handleDirtyCoordinates();

private:
	void onLoadedChunk(WorldChunk &chunk);

#pragma region Dirty Blocks
private:
	typedef std::pair<
		world::Coordinate,
		std::vector<world::Coordinate>
	> DirtyNeighborPair;
	typedef std::vector<DirtyNeighborPair> DirtyNeighborPairList;

	std::vector<WorldChunk> mActiveChunks;
	/**
	 * Map of neighbor coordinate to coordinates which received an update.
	 */
	DirtyNeighborPairList mDirtyNeighborCoordinates;

	void markCoordinateWithDirtyNeighbor(
		world::Coordinate const &global,
		world::Coordinate const &dirtyNeighbor
	);
	DirtyNeighborPairList::iterator findDirtyNeighborPair(world::Coordinate const &global);
#pragma endregion

};

NS_END
