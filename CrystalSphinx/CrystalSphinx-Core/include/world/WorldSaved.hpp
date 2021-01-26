#pragma once

#include "world/World.hpp"

#include "world/WorldSaveData.hpp"

FORWARD_DEF(NS_SAVE_DATA, class Instance);

NS_WORLD

class WorldSaved : public World
{

public:
	WorldSaved(saveData::Instance* saveInstance);

	saveData::Instance* getSaveInstance() const;

	/**
	 * Returns a non-deterministic location
	 * to spawn a player around the spawn chunk.
	 */
	world::Coordinate makeSpawnLocation() const;

	void init() override;
	void loadChunk(DimensionId const& dimId, math::Vector3Int const& coord) override;

private:
	saveData::Instance* mpSaveInstance;
	world::SaveData mSaveData;

	ui32 seed() const;

	/**
	 * Uses deterministic random to return the chunk coordinate of spawn.
	 */
	math::Vector2Int getSpawnChunkCoord() const;

};

NS_END
