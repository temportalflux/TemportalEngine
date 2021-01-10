#pragma once

#include "CoreInclude.hpp"

NS_ECS
FORWARD_DEF(NS_SYSTEM, class PhysicsIntegration);
NS_END
FORWARD_DEF(NS_PHYSICS, class Material);
FORWARD_DEF(NS_PHYSICS, class RigidBody);
FORWARD_DEF(NS_PHYSICS, class Scene);
FORWARD_DEF(NS_PHYSICS, class System);
FORWARD_DEF(NS_PHYSICS, class ChunkCollisionManager);
FORWARD_DEF(NS_WORLD, class World);

NS_GAME
class VoxelTypeRegistry;

class WorldLogic
{

public:
	std::shared_ptr<game::VoxelTypeRegistry> voxelTypeRegistry() { return this->mpVoxelTypeRegistry; }

	void init();
	void uninit();
	void update(f32 deltaTime);

private:

	std::shared_ptr<game::VoxelTypeRegistry> mpVoxelTypeRegistry;
	void createVoxelTypeRegistry();

	std::shared_ptr<physics::System> mpPhysics;
	std::shared_ptr<physics::Scene> mpSceneOverworld;
	std::shared_ptr<physics::Material> mpPlayerPhysicsMaterial;
	std::shared_ptr<physics::ChunkCollisionManager> mpChunkCollisionManager; // for Overworld only

	std::shared_ptr<ecs::system::PhysicsIntegration> mpSystemPhysicsIntegration;

	std::shared_ptr<world::World> mpWorld;

	void createScene();
	void destroyScene();
	void createWorld();
	void destroyWorld();

};

NS_END
