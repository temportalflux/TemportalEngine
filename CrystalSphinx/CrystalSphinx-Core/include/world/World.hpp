#pragma once

#include "CoreInclude.hpp"

#include "ecs/types.h"
#include "world/WorldCoordinate.hpp"

NS_ECS
FORWARD_DEF(NS_SYSTEM, class PhysicsIntegration);
NS_END
FORWARD_DEF(NS_PHYSICS, class Material);
FORWARD_DEF(NS_PHYSICS, class RigidBody);
FORWARD_DEF(NS_PHYSICS, class Scene);
FORWARD_DEF(NS_PHYSICS, class System);
FORWARD_DEF(NS_PHYSICS, class ChunkCollisionManager);
FORWARD_DEF(NS_WORLD, class Terrain);
FORWARD_DEF(NS_WORLD, class SaveData);

NS_GAME
class VoxelTypeRegistry;

class World
{

public:
	World();

	std::shared_ptr<game::VoxelTypeRegistry> voxelTypeRegistry();
	std::shared_ptr<physics::Material> playerPhysicsMaterial();
	std::shared_ptr<physics::Scene> dimensionScene(ui32 dimId);

	void setSaveData(world::SaveData* pSaveData);
	void init();
	void uninit();
	void update(f32 deltaTime);

	/**
	 * Creates a player entity (though doesn't include any rendering or POV components/views).
	 * Returns the EVCS entity id.
	 */
	ecs::Identifier createPlayer(ui32 clientNetId);
	void destroyPlayer(ecs::Identifier entityId);

private:
	// If this world owns the save data, this is non-null.
	// Otherwise, the world is replicated from some server.
	world::SaveData* mpSaveData;

	ui32 seed() const;

	/**
	 * Uses deterministic random to return the chunk coordinate of spawn.
	 */
	math::Vector2Int getSpawnChunkCoord(ui32 seed) const;
	/**
	 * Returns a non-deterministic location
	 * to spawn a player around the spawn chunk.
	 */
	world::Coordinate makeSpawnLocation(ui32 seed) const;

	std::shared_ptr<game::VoxelTypeRegistry> mpVoxelTypeRegistry;
	void createVoxelTypeRegistry();

	std::shared_ptr<physics::System> mpPhysics;
	std::shared_ptr<physics::Material> mpPlayerPhysicsMaterial;
	std::shared_ptr<ecs::system::PhysicsIntegration> mpSystemPhysicsIntegration;
	void initializePhysics();
	void uninitializePhysics();

	struct Dimension
	{
		ui32 id;
		std::shared_ptr<physics::Scene> mpScene;
		std::shared_ptr<world::Terrain> mpTerrain;
		std::shared_ptr<physics::ChunkCollisionManager> mpChunkCollisionManager;
	};
	Dimension mOverworld;
	void createDimension(Dimension *dim);
	void destroyDimension(Dimension *dim);

};

NS_END
