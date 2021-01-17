#pragma once

#include "CoreInclude.hpp"

#include "world/WorldSaveData.hpp"
#include "ecs/types.h"

NS_ECS
FORWARD_DEF(NS_SYSTEM, class PhysicsIntegration);
NS_END
FORWARD_DEF(NS_PHYSICS, class Material);
FORWARD_DEF(NS_PHYSICS, class RigidBody);
FORWARD_DEF(NS_PHYSICS, class Scene);
FORWARD_DEF(NS_PHYSICS, class System);
FORWARD_DEF(NS_PHYSICS, class ChunkCollisionManager);
FORWARD_DEF(NS_WORLD, class Terrain);

namespace saveData { class Instance; }

NS_GAME
class VoxelTypeRegistry;

class World
{

public:
	World();

	world::SaveData& saveData();
	std::shared_ptr<game::VoxelTypeRegistry> voxelTypeRegistry();

	void init(saveData::Instance* saveInstance);
	void uninit();
	void update(f32 deltaTime);

	/**
	 * Creates a player entity (though doesn't include any rendering or POV components/views).
	 * Returns the EVCS entity id.
	 */
	ecs::Identifier createPlayer();
	void destroyPlayer(ecs::Identifier entityId);

private:
	saveData::Instance* mpSaveInstance;
	world::SaveData mSaveData;

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
