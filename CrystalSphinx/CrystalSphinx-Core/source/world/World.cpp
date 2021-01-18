#include "world/World.hpp"

#include "Engine.hpp"
#include "asset/BlockType.hpp"
#include "ecs/component/CoordinateTransform.hpp"
#include "ecs/component/ComponentPlayerPhysics.hpp"
#include "ecs/system/SystemPhysicsIntegration.hpp"
#include "game/GameInstance.hpp"
#include "physics/ChunkCollisionManager.hpp"
#include "physics/PhysicsMaterial.hpp"
#include "physics/PhysicsRigidBody.hpp"
#include "physics/PhysicsScene.hpp"
#include "physics/PhysicsShape.hpp"
#include "physics/PhysicsSystem.hpp"
#include "registry/VoxelType.hpp"
#include "saveData/SaveDataRegistry.hpp"
#include "world/WorldSaveData.hpp"
#include "world/WorldTerrain.hpp"

using namespace world;

logging::Logger WORLD_LOG = DeclareLog("World", LOG_INFO);

World::World()
{
}

World::~World()
{
}

void World::init()
{
	this->createVoxelTypeRegistry();
	this->initializePhysics();
	this->mOverworld.id = 0;
	this->createDimension(&this->mOverworld);
}

void World::uninit()
{
	this->destroyDimension(&this->mOverworld);
	this->uninitializePhysics();
	this->mpVoxelTypeRegistry.reset();
	ecs::Core::Get()->entities().releaseAll();
}

std::shared_ptr<game::VoxelTypeRegistry> World::voxelTypeRegistry() { return this->mpVoxelTypeRegistry; }
std::shared_ptr<physics::Material> World::playerPhysicsMaterial() { return this->mpPlayerPhysicsMaterial; }
std::shared_ptr<physics::Scene> World::dimensionScene(ui32 dimId) { return this->mOverworld.mpScene; }

void World::createVoxelTypeRegistry()
{
	this->mpVoxelTypeRegistry = std::make_shared<game::VoxelTypeRegistry>();

	WORLD_LOG.log(LOG_INFO, "Gathering block types...");
	auto blockList = engine::Engine::Get()->getAssetManager()->getAssetList<asset::BlockType>();
	WORLD_LOG.log(LOG_INFO, "Found %i block types", blockList.size());
	this->mpVoxelTypeRegistry->registerEntries(blockList);
}

void World::initializePhysics()
{
	WORLD_LOG.log(LOG_INFO, "Initializing physics");
	this->mpPhysics = std::make_shared<physics::System>();
	this->mpPhysics->init(true);

	this->mpPlayerPhysicsMaterial = std::make_shared<physics::Material>();
	this->mpPlayerPhysicsMaterial->setSystem(this->mpPhysics);
	this->mpPlayerPhysicsMaterial->create();

	this->mpSystemPhysicsIntegration = std::make_shared<ecs::system::PhysicsIntegration>();
	engine::Engine::Get()->addTicker(this->mpSystemPhysicsIntegration);
}

void World::uninitializePhysics()
{
	WORLD_LOG.log(LOG_INFO, "Destroying physics");
	this->mpSystemPhysicsIntegration.reset();
	this->mpPlayerPhysicsMaterial.reset();
	this->mpPhysics.reset();
}

void World::createDimension(Dimension *dim)
{
	WORLD_LOG.log(LOG_INFO, "Creating dimension %u", dim->id);
	dim->mpScene = std::make_shared<physics::Scene>();
	dim->mpScene->setSystem(this->mpPhysics);
	dim->mpScene->create();

	dim->mpChunkCollisionManager = std::make_shared<physics::ChunkCollisionManager>(
		this->mpPhysics, dim->mpScene
	);

	dim->mpTerrain = std::make_shared<world::Terrain>();
	dim->mpTerrain->addEventListener(dim->mpChunkCollisionManager);

	//this->mpTerrain->loadChunk({ 0, 0, 0 });
	//this->mpTerrain->loadChunk({ 0, 0, -1 });
}

void World::destroyDimension(Dimension *dim)
{
	WORLD_LOG.log(LOG_INFO, "Destroying dimension %u", dim->id);
	dim->mpTerrain.reset();
	dim->mpChunkCollisionManager.reset();
	dim->mpScene.reset();
}

ecs::Identifier World::createPlayer(ui32 clientNetId, world::Coordinate const& position)
{
	auto& ecs = engine::Engine::Get()->getECS();

	// Turning on replication if on a dedicated or integrated server.
	// If replication is on, ecs will auto-generate packets.
	if (game::Game::networkInterface()->type().includes(network::EType::eServer))
	{
		ecs.beginReplication();
	}

	// does not mark the entity to be killed, so the manager will own it.
	// can look up the entity by id by using `EntityManager#get`.
	auto pEntity = ecs.entities().create();
	pEntity->setOwner(clientNetId);

	// Add Transform
	{
		// TODO: Load player location and rotation from save data
		auto transform = ecs.components().create<ecs::component::CoordinateTransform>();
		transform->setPosition(position);
		transform->setOrientation(math::Vector3unitY, 0); // force the camera to face forward (-Z)
		pEntity->addComponent(transform);
	}

	// Add physics
	{
		auto physics = ecs.components().create<ecs::component::PlayerPhysics>();
		pEntity->addComponent(physics);
	}

	WORLD_LOG.log(LOG_INFO, "Created player entity id(%u)", pEntity->id());
	
	// End replication only does anything if `beginReplication` is called.
	// If `beginReplication` is called, `endReplication` MUST be called.
	// Will broadcast packets to all network connections.
	ecs.endReplication();
	return pEntity->id();
}

void World::destroyPlayer(ecs::Identifier entityId)
{
	auto& ecs = engine::Engine::Get()->getECS();
	if (game::Game::networkInterface()->type().includes(network::EType::eServer))
	{
		ecs.beginReplication();
	}
	ecs.entities().release(entityId);
	ecs.endReplication();
}

void World::addTerrainEventListener(ui32 dimId, std::shared_ptr<WorldEventListener> listener)
{
	this->mOverworld.mpTerrain->addEventListener(listener);
}

void World::removeTerrainEventListener(ui32 dimId, std::shared_ptr<WorldEventListener> listener)
{
	this->mOverworld.mpTerrain->removeEventListener(listener);
}

void World::update(f32 deltaTime)
{
	this->mOverworld.mpScene->simulate(deltaTime);
}
