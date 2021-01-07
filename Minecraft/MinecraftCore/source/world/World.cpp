#include "world/World.hpp"

using namespace world;

World::World(ui32 seed)
	: mSeed(seed)
	, mSpawnChunkCoord(makeSpawnChunkCoord(seed))
	, mSpawnAreaChunkRadius(2)
{
}

World::~World()
{
}

math::Vector2Int randInRange(math::Vector2Int const& rand, i32 range)
{
	auto const min = math::Vector2Int(-range);
	auto const max = math::Vector2Int(range);
	return (rand % (max - min)) + min;
}

math::Vector2Int World::makeSpawnChunkCoord(ui32 seed)
{
	srand(seed);
	static i32 SPAWN_AREA_CENTER_RANGE = 5;
	return randInRange({ rand(), rand() }, SPAWN_AREA_CENTER_RANGE);
}

world::Coordinate World::makeSpawnLocation() const
{
	srand((ui32)time(0));
	auto chunkXZ = this->mSpawnChunkCoord + randInRange({ rand(), rand() }, (i32)this->mSpawnAreaChunkRadius);
	auto chunk = math::Vector3Int({ chunkXZ.x(), 0,chunkXZ.y() });
	auto block = math::Vector3Int({ rand() % CHUNK_SIDE_LENGTH, 0, rand() % CHUNK_SIDE_LENGTH });

	// TODO: raycast down to find y-coord of both chunk and block
	chunk.y() = 0;
	block.y() = 2;

	return world::Coordinate(chunk, block);
}

void World::addEventListener(std::shared_ptr<WorldEventListener> listener)
{
	this->OnLoadingChunk.bind(listener, listener->onLoadingChunkEvent());
	this->OnUnloadingChunk.bind(listener, listener->onUnloadingChunkEvent());
	this->OnVoxelsChanged.bind(listener, listener->onVoxelsChangedEvent());
}

Chunk* World::getOrLoadChunk(math::Vector3Int const& coordinate)
{
	if (auto chunk = this->findChunk(coordinate))
	{
		return chunk;
	}
	else
	{
		return this->loadChunk(coordinate);
	}
}

Chunk* World::findChunk(math::Vector3Int const& coordinate)
{
	for (auto& activeChunk : this->mActiveChunks)
	{
		if (activeChunk.coordinate() == coordinate)
		{
			return &activeChunk;
		}
	}
	return nullptr;
}

Chunk* World::loadChunk(math::Vector3Int const &coordinate)
{
	this->OnLoadingChunk.broadcast(coordinate);
	return &(this->mActiveChunks.insert(
		this->mActiveChunks.end(),
		Chunk(this->weak_from_this(), coordinate)
	)->load());
}

void World::reloadChunk(math::Vector3Int const &coordinate)
{
	/*
	this->unloadChunk(coordinate);
	this->loadChunk(coordinate);
	//*/
	///*
	for (auto& activeChunk : this->mActiveChunks)
	{
		if (activeChunk.coordinate() == coordinate)
		{
			activeChunk.load();
			return;
		}
	}
	//*/
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

	for (auto const& coordinate : coordinates)
	{
		this->OnUnloadingChunk.broadcast(coordinate);
	}
	// TODO: Move the active chunk to a "pending write to disk" operation list
}

void World::onLoadedChunk(Chunk &chunk)
{
	auto changes = std::vector<std::pair<world::Coordinate, std::optional<game::BlockId>>>();
	for (auto iter = chunk.begin(); iter != chunk.end(); ++iter)
	{
		auto entry = *iter;
		auto coord = world::Coordinate{ chunk.coordinate(), entry.localCoord };
		std::optional<game::BlockId> voxelId = std::nullopt;
		if (entry.data) voxelId = entry.data->id;
		changes.push_back(std::make_pair(coord, voxelId));
	}
	this->OnVoxelsChanged.broadcast(changes);
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
	//this->OnBlockChanged.broadcast(global, prev, next);
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
	OPTICK_EVENT();
	// TODO: actually do something here
	this->mDirtyNeighborCoordinates.clear();
}
