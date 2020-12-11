#pragma once

#include "CoreInclude.hpp"

#include "utility/CoordMap.hpp"
#include "world/BlockMetadata.hpp"

FORWARD_DEF(NS_WORLD, class World);

/**
 * The structure representing a currently loaded chunk.
 */
class WorldChunk
{
public:
	typedef CoordMap<std::optional<BlockMetadata>, CHUNK_SIDE_LENGTH> TCoordData;

	WorldChunk(std::weak_ptr<world::World> world, math::Vector3Int coordinate);

	math::Vector3Int const& coordinate() const;

	void load();

	void setBlockId(math::Vector3Int const local, std::optional<game::BlockId> id);

	std::optional<game::BlockId> getBlockId(math::Vector3Int const local) const
	{
		auto& metadata = this->mBlockMetadata[local];
		return metadata ? std::make_optional(metadata->id) : std::nullopt;
	}

	TCoordData::iterator begin() { return this->mBlockMetadata.begin(); }
	TCoordData::iterator end() { return this->mBlockMetadata.end(); }
	TCoordData::const_iterator begin() const { return this->mBlockMetadata.begin(); }
	TCoordData::const_iterator end() const { return this->mBlockMetadata.end(); }

private:
	std::weak_ptr<world::World> mpWorld;
	math::Vector3Int mCoordinate;
	TCoordData mBlockMetadata;


};
