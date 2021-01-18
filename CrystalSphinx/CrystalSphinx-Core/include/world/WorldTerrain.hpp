#pragma once

#include "CoreInclude.hpp"

#include "Delegate.hpp"

#include "world/WorldChunk.hpp"
#include "world/WorldCoordinate.hpp"
#include "world/Events.hpp"

NS_WORLD

class Terrain : public std::enable_shared_from_this<Terrain>
{
	friend class Chunk;

public:
	Terrain();
	~Terrain();

	TOnChunkLoadingEvent OnLoadingChunk, OnUnloadingChunk;
	TOnVoxelsChangedEvent OnVoxelsChanged;
	void addEventListener(std::shared_ptr<WorldEventListener> listener);
	void removeEventListener(std::shared_ptr<WorldEventListener> listener);

	Chunk* getOrLoadChunk(math::Vector3Int const& coordinate);
	Chunk* loadChunk(math::Vector3Int const &coordinate);
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

	Chunk* findChunk(math::Vector3Int const& coordinate);
	void onLoadedChunk(Chunk &chunk);

#pragma region Dirty Blocks
private:
	typedef std::pair<
		world::Coordinate,
		std::vector<world::Coordinate>
	> DirtyNeighborPair;
	typedef std::vector<DirtyNeighborPair> DirtyNeighborPairList;

	std::vector<Chunk> mActiveChunks;
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
