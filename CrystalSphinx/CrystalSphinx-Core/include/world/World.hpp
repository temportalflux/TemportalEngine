#pragma once

#include "CoreInclude.hpp"

#include "ecs/types.h"
#include "world/Events.hpp"
#include "world/WorldCoordinate.hpp"
#include "physics/PhysicsController.hpp"

NS_ECS
FORWARD_DEF(NS_SYSTEM, class PhysicsIntegration);
NS_END
FORWARD_DEF(NS_GAME, class VoxelTypeRegistry);
FORWARD_DEF(NS_PHYSICS, class Material);
FORWARD_DEF(NS_PHYSICS, class RigidBody);
FORWARD_DEF(NS_PHYSICS, class Scene);
FORWARD_DEF(NS_PHYSICS, class System);
FORWARD_DEF(NS_PHYSICS, class ChunkCollisionManager);
FORWARD_DEF(NS_WORLD, class Terrain);

NS_WORLD

class World : public virtual_enable_shared_from_this<World>
{

public:
	World();
	virtual ~World();

	std::shared_ptr<game::VoxelTypeRegistry> voxelTypeRegistry();
	std::shared_ptr<physics::Material> playerPhysicsMaterial();
	std::shared_ptr<physics::Scene> dimensionScene(ui32 dimId);
	void addTerrainEventListener(ui32 dimId, std::shared_ptr<WorldEventListener> listener);
	void removeTerrainEventListener(ui32 dimId, std::shared_ptr<WorldEventListener> listener);

	void init();
	void uninit();
	void update(f32 deltaTime);

	/**
	 * Creates a player entity (though doesn't include any rendering or POV components/views).
	 * Returns the EVCS entity id.
	 */
	ecs::Identifier createPlayer(ui32 clientNetId, world::Coordinate const& position);
	void destroyPlayer(ui32 userNetId, ecs::Identifier entityId);

	void createPlayerController(ui32 userNetId, ecs::Identifier localEntityId);
	void destroyPlayerController(ui32 userNetId);

protected:
	struct Dimension
	{
		ui32 id;
		std::shared_ptr<physics::Scene> mpScene;
		std::shared_ptr<world::Terrain> mpTerrain;
		std::shared_ptr<physics::ChunkCollisionManager> mpChunkCollisionManager;
	};

	virtual void loadChunk(Dimension &dim, math::Vector3Int coord) {}

private:

	std::shared_ptr<game::VoxelTypeRegistry> mpVoxelTypeRegistry;
	void createVoxelTypeRegistry();

	std::shared_ptr<physics::System> mpPhysics;
	std::shared_ptr<physics::Material> mpPlayerPhysicsMaterial;
	std::shared_ptr<ecs::system::PhysicsIntegration> mpSystemPhysicsIntegration;
	void initializePhysics();
	void uninitializePhysics();

	Dimension mOverworld;
	void createDimension(Dimension *dim);
	void destroyDimension(Dimension *dim);

	std::map<ui32, physics::Controller> mPhysicsControllerByUserNetId;

};

NS_END
