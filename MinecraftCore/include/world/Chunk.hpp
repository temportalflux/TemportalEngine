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
	WorldChunk(std::weak_ptr<world::World> world, math::Vector3Int coordinate);

	math::Vector3Int const& coordinate() const;

	void load();

	void setBlockId(math::Vector3UInt const local, std::optional<game::BlockId> id);

	std::optional<game::BlockId> getBlockId(math::Vector3UInt const local) const
	{
		auto& metadata = this->mBlockMetadata[local];
		return metadata ? std::make_optional(metadata->id) : std::nullopt;
	}

private:
	std::weak_ptr<world::World> mpWorld;
	math::Vector3Int mCoordinate;
	CoordMap<std::optional<BlockMetadata>, 16> mBlockMetadata;


};
