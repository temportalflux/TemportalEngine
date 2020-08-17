#pragma once

#include "CoreInclude.hpp"

#include "world/Chunk.hpp"
#include "world/WorldCoordinate.hpp"

NS_WORLD

class World : public std::enable_shared_from_this<World>
{

public:

	void loadChunk(math::Vector3Int const &coordinate);
	void unloadChunk(math::Vector3Int const &coordinate);
	void unloadChunks(std::vector<math::Vector3Int> coordinates);

	void markCoordinateDirty(world::Coordinate const &global);

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

};

NS_END
