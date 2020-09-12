#include "world/Chunk.hpp"

#include "world/World.hpp"
#include "world/WorldCoordinate.hpp"

WorldChunk::WorldChunk(
	std::weak_ptr<world::World> world, math::Vector3Int coordinate
)
	: mpWorld(world)
	, mCoordinate(coordinate)
{
}

math::Vector3Int const& WorldChunk::coordinate() const
{
	return this->mCoordinate;
}

// TODO: Save and load from file
void WorldChunk::load()
{
	for (ui32 x = 0; x < world::ChunkSize(); ++x)
	{
		for (ui32 y = 0; y < world::ChunkSize(); ++y)
		{
			this->setBlockId({ x, y, 0 }, game::BlockId("minecraft", "dirt"));
		}
	}
}

void WorldChunk::setBlockId(math::Vector3UInt const local, std::optional<game::BlockId> id)
{
	std::optional<BlockMetadata>& metadata = this->mBlockMetadata[local];
	auto oldMetadata = metadata;
	metadata.reset();
	if (id)
	{
		metadata = BlockMetadata(*id);
	}
	this->mpWorld.lock()->markCoordinateDirty(world::Coordinate(
		this->coordinate(), { (i32)local.x(), (i32)local.y(), (i32)local.z() }
	), oldMetadata, metadata);
}
