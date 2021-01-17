#pragma once

#include "CoreInclude.hpp"

#include "utility/CoordMap.hpp"
#include "world/BlockMetadata.hpp"

FORWARD_DEF(NS_WORLD, class Terrain);

NS_WORLD
class Chunk;

enum class EChunkLoadTier : ui8
{
	eActive = 0,
	eTicking = 1,
	eLoaded = 2,

	eInactive,
	COUNT = eInactive,
};

class ChunkHandle
{
	friend class Chunk;

public:
	ChunkHandle();
	ChunkHandle(ChunkHandle const& other) = delete;
	ChunkHandle(ChunkHandle &&other);
	ChunkHandle& operator=(ChunkHandle &&other);
	~ChunkHandle();
	void release();

private:
	ChunkHandle(Chunk* chunk, EChunkLoadTier tier);

	Chunk* mpChunk;
	EChunkLoadTier mTier;
};

/**
 * The structure representing a currently loaded chunk.
 */
class Chunk
{
public:
	typedef CoordMap<std::optional<BlockMetadata>, CHUNK_SIDE_LENGTH> TCoordData;

	Chunk(std::weak_ptr<world::Terrain> world, math::Vector3Int coordinate);

	math::Vector3Int const& coordinate() const;

	void setBlockId(math::Vector3Int const local, std::optional<game::BlockId> id);
	std::optional<game::BlockId> getBlockId(math::Vector3Int const local) const;

	TCoordData::iterator begin() { return this->mBlockMetadata.begin(); }
	TCoordData::iterator end() { return this->mBlockMetadata.end(); }
	TCoordData::const_iterator begin() const { return this->mBlockMetadata.begin(); }
	TCoordData::const_iterator end() const { return this->mBlockMetadata.end(); }

	void generate();

	Chunk& load();
	void createLoadHandle(EChunkLoadTier tier, ChunkHandle &out);
	void releaseLoadHandle(EChunkLoadTier tier);
	bool shouldStayLoaded() const;

private:
	std::weak_ptr<world::Terrain> mpTerrain;
	math::Vector3Int mCoordinate;
	TCoordData mBlockMetadata;

	std::array<ui32, (uSize)EChunkLoadTier::COUNT> mChunkLoadFlags;
	EChunkLoadTier mLoadTier;
	void updateLoadTier(EChunkLoadTier tier, i32 delta);
	ui32& loadTierCount(EChunkLoadTier tier);

};

NS_END
