#include "world/WorldSaved.hpp"

#include "saveData/SaveDataRegistry.hpp"
#include "utility/Random.hpp"

using namespace world;

WorldSaved::WorldSaved(saveData::Instance* saveInstance)
	: World()
	, mpSaveInstance(saveInstance)
	, mSaveData(saveInstance->worldSave())
{
	this->mSaveData.readFromDisk();
}

saveData::Instance* WorldSaved::getSaveInstance() const
{
	return this->mpSaveInstance;
}

ui32 WorldSaved::seed() const
{
	return this->mSaveData.seed();
}

math::Vector2Int WorldSaved::getSpawnChunkCoord() const
{
	auto random = utility::Random(this->seed());
	// returns a <x,z> within <[-2, 2], [-2, 2]>
	// will be the same chunk for a given seed
	return random.nextVInS<2>(2);
}

world::Coordinate WorldSaved::makeSpawnLocation() const
{
	// using time as seed - not deterministic
	auto random = utility::Random((ui32)time(0));

	// returns a <x,z> within <[-3, 3], [-3, 3]>
	auto offset = random.nextVInS<2>(3);
	auto chunkXZ = this->getSpawnChunkCoord() + offset;
	auto voxelXZ = random.nextVIn<2>(0, CHUNK_SIDE_LENGTH);

	auto chunkPos = math::Vector3Int({ chunkXZ.x(), 0, chunkXZ.y() });
	auto voxelPos = math::Vector3Int({ voxelXZ.x(), 0, voxelXZ.y() });

	// TODO: raycast down to find y-coord of both chunk and block
	chunkPos.y() = 0;
	voxelPos.y() = 2;

	return world::Coordinate(chunkPos, voxelPos);
}

void WorldSaved::loadChunk(Dimension &dim, math::Vector3Int coord)
{
	// TODO
}
