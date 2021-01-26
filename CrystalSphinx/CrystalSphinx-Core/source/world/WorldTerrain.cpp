#include "world/WorldTerrain.hpp"

using namespace world;

Terrain::Terrain()
{
}

Terrain::~Terrain()
{
}

void Terrain::addEventListener(std::shared_ptr<WorldEventListener> listener)
{
	this->OnLoadingChunk.bind(listener, listener->onLoadingChunkEvent());
	this->OnUnloadingChunk.bind(listener, listener->onUnloadingChunkEvent());
	this->OnVoxelsChanged.bind(listener, listener->onVoxelsChangedEvent());
}

void Terrain::removeEventListener(std::shared_ptr<WorldEventListener> listener)
{
	this->OnLoadingChunk.unbind(listener);
	this->OnUnloadingChunk.unbind(listener);
	this->OnVoxelsChanged.unbind(listener);
}

Chunk* Terrain::getChunk(math::Vector3Int const& coordinate)
{
	auto pChunk = this->findChunk(coordinate);
	assert(pChunk != nullptr);
	return pChunk;
}

Chunk* Terrain::getOrLoadChunk(math::Vector3Int const& coordinate)
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

Chunk* Terrain::findChunk(math::Vector3Int const& coordinate)
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

Chunk* Terrain::loadChunk(math::Vector3Int const &coordinate)
{
	auto pChunk = this->startLoadingChunk(coordinate);
	pChunk->load();
	return pChunk;
}

Chunk* Terrain::startLoadingChunk(math::Vector3Int const& coordinate)
{
	this->OnLoadingChunk.broadcast(coordinate);
	auto iter = this->mActiveChunks.insert(
		this->mActiveChunks.end(),
		Chunk(this->weak_from_this(), coordinate)
	);
	return &(*iter);
}

void Terrain::finishLoadingChunk(Chunk* pChunk)
{
	this->onLoadedChunk(*pChunk);
}

void Terrain::reloadChunk(math::Vector3Int const &coordinate)
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

void Terrain::unloadChunk(math::Vector3Int const &coordinate)
{
	this->unloadChunks({ coordinate });
}

void Terrain::unloadChunks(std::vector<math::Vector3Int> coordinates)
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

void Terrain::onLoadedChunk(Chunk &chunk)
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

void Terrain::markCoordinateDirty(
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

void Terrain::markCoordinateWithDirtyNeighbor(world::Coordinate const &global, world::Coordinate const &dirtyNeighbor)
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

Terrain::DirtyNeighborPairList::iterator Terrain::findDirtyNeighborPair(world::Coordinate const &global)
{
	return std::find_if(
		this->mDirtyNeighborCoordinates.begin(),
		this->mDirtyNeighborCoordinates.end(),
		[global](auto const& dirtyPair) { return dirtyPair.first == global; }
	);
}

void Terrain::handleDirtyCoordinates()
{
	OPTICK_EVENT();
	// TODO: actually do something here
	this->mDirtyNeighborCoordinates.clear();
}
