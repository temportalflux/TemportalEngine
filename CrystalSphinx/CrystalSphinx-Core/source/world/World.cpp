#include "world/World.hpp"

#include "Engine.hpp"
#include "asset/BlockType.hpp"
#include "ecs/component/CoordinateTransform.hpp"
#include "ecs/component/ComponentPlayerPhysics.hpp"
#include "ecs/system/SystemPhysicsIntegration.hpp"
#include "ecs/system/SystemIntegratePlayerPhysics.hpp"
#include "ecs/view/ViewPlayerPhysics.hpp"
#include "game/GameInstance.hpp"
#include "physics/ChunkCollisionManager.hpp"
#include "physics/PhysicsMaterial.hpp"
#include "physics/PhysicsRigidBody.hpp"
#include "physics/PhysicsScene.hpp"
#include "physics/PhysicsShape.hpp"
#include "physics/PhysicsSystem.hpp"
#include "saveData/SaveDataRegistry.hpp"
#include "world/WorldSaveData.hpp"
#include "world/WorldTerrain.hpp"

using namespace world;

logging::Logger WORLD_LOG = DeclareLog("World", LOG_INFO);

World::World()
	: mSimulationFrequency(1.0f / 60.0f)
	, mTimeSinceLastSimulate(0.0f)
{
}

World::~World()
{
}

void World::init()
{
	this->initializePhysics();
	this->mOverworld.id = 0;
	this->createDimension(&this->mOverworld);
}

void World::uninit()
{
	// Destroy any lingering physics::Controller objects
	this->mPhysicsControllerByUserNetId.clear();
	this->destroyDimension(&this->mOverworld);
	this->uninitializePhysics();
	evcs::Core::Get()->entities().releaseAll();
}

std::shared_ptr<physics::Material> World::playerPhysicsMaterial() { return this->mpPlayerPhysicsMaterial; }
std::shared_ptr<physics::Scene> World::dimensionScene(DimensionId dimId) { return this->dimension(dimId).mpScene; }
std::shared_ptr<world::Terrain> World::terrain(DimensionId dimId) { return this->dimension(dimId).mpTerrain; }

void World::initializePhysics()
{
	WORLD_LOG.log(LOG_INFO, "Initializing physics");
	this->mpPhysics = std::make_shared<physics::System>();
	this->mpPhysics->init(this->shouldConnectToPhysxDebugger());

	this->mpPlayerPhysicsMaterial = std::make_shared<physics::Material>();
	this->mpPlayerPhysicsMaterial->setSystem(this->mpPhysics);
	this->mpPlayerPhysicsMaterial->create();

	this->mpSystemPhysicsIntegration = std::make_shared<evcs::system::PhysicsIntegration>();	
	this->mpSystemIntegratePlayerPhysics = std::make_shared<evcs::system::IntegratePlayerPhysics>();
}

void World::uninitializePhysics()
{
	WORLD_LOG.log(LOG_INFO, "Destroying physics");
	this->mpSystemIntegratePlayerPhysics.reset();
	this->mpSystemPhysicsIntegration.reset();
	this->mpPlayerPhysicsMaterial.reset();
	this->mpPhysics.reset();
}

f32 const& World::simulationFrequency() const
{
	return this->mSimulationFrequency;
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
}

World::Dimension& World::dimension(DimensionId const& dimId)
{
	return this->mOverworld;
}

void World::destroyDimension(Dimension *dim)
{
	WORLD_LOG.log(LOG_INFO, "Destroying dimension %u", dim->id);
	dim->mpTerrain.reset();
	dim->mpChunkCollisionManager.reset();
	dim->mpScene.reset();
}

void World::addTerrainEventListener(ui32 dimId, std::shared_ptr<WorldEventListener> listener)
{
	this->dimension(dimId).mpTerrain->addEventListener(listener);
}

void World::removeTerrainEventListener(ui32 dimId, std::shared_ptr<WorldEventListener> listener)
{
	this->dimension(dimId).mpTerrain->removeEventListener(listener);
}

evcs::Identifier World::createPlayer(ui32 clientNetId, world::Coordinate const& position)
{
	auto& ecs = engine::Engine::Get()->getECS();

	// Turning on replication if on a dedicated or integrated server.
	// If replication is on, ecs will auto-generate packets.
	if (game::Game::networkInterface()->type().includes(network::EType::eServer))
	{
		// TODO: Update replication list so that order of object modification doesnt matter.
		// Right now, updating a component after it is assigned a view will result in the component
		// not being created until after the view is created.
		// The replicator list needs to re-sort itself based on what packets require what other packets.
		ecs.beginReplication();
	}

	// does not mark the entity to be killed, so the manager will own it.
	// can look up the entity by id by using `EntityManager#get`.
	auto pEntity = ecs.entities().create(true);

	// Add Transform
	{
		auto transform = ecs.components().create<evcs::component::CoordinateTransform>(true);
		transform->setPosition(position);
		transform->setOrientation(math::Vector3unitY, 0); // force the camera to face forward (-Z)
		pEntity->addComponent(transform);
	}

	// Add physics
	{
		auto physics = ecs.components().create<evcs::component::PlayerPhysics>(true);
		physics->setIsAffectedByGravity(false);
		pEntity->addComponent(physics);

		pEntity->addView(ecs.views().create<evcs::view::PlayerPhysics>(true));
	}

	pEntity->setOwner(clientNetId);
	this->createPlayerController(clientNetId, pEntity->id());

	WORLD_LOG.log(LOG_INFO, "Created player entity id(%u)", pEntity->id());
	
	// End replication only does anything if `beginReplication` is called.
	// If `beginReplication` is called, `endReplication` MUST be called.
	// Will broadcast packets to all network connections.
	ecs.endReplication();
	return pEntity->id();
}

void World::destroyPlayer(ui32 userNetId, evcs::Identifier entityId)
{
	auto& ecs = engine::Engine::Get()->getECS();
	this->destroyPlayerController(userNetId);
	if (game::Game::networkInterface()->type().includes(network::EType::eServer))
	{
		ecs.beginReplication();
	}
	ecs.entities().release(entityId);
	ecs.endReplication();
}

void World::createPlayerController(network::Identifier userNetId, evcs::Identifier localEntityId)
{
	WORLD_LOG.log(LOG_VERBOSE, "Creating physics controller for player net(%u)", userNetId);

	auto& ecs = engine::Engine::Get()->getECS();
	auto pEntity = ecs.entities().get(localEntityId);
	auto* pTransform = pEntity->getComponent<evcs::component::CoordinateTransform>();
	auto* pPhysics = pEntity->getComponent<evcs::component::PlayerPhysics>();

	auto iter = this->mPhysicsControllerByUserNetId.insert(std::make_pair(
		userNetId, std::move(physics::Controller())
	));
	auto const& extents = pPhysics->collisionExtents();
	iter.first->second
		.setScene(this->mOverworld.mpScene)
		.setAsBox(extents)
		.setCenterPosition(pTransform->position().toGlobal() + math::Vector<f64, 3>({ 0, extents.y(), 0 }))
		.setMaterial(this->playerPhysicsMaterial().get())
		.create();
}

bool World::hasPhysicsController(network::Identifier userNetId) const
{
	return this->mPhysicsControllerByUserNetId.find(userNetId) != this->mPhysicsControllerByUserNetId.end();
}

physics::Controller& World::getPhysicsController(network::Identifier userNetId)
{
	auto iter = this->mPhysicsControllerByUserNetId.find(userNetId);
	assert(iter != this->mPhysicsControllerByUserNetId.end());
	return iter->second;
}

void World::destroyPlayerController(network::Identifier userNetId)
{
	WORLD_LOG.log(LOG_VERBOSE, "Destroying physics controller for player net(%u)", userNetId);
	auto iter = this->mPhysicsControllerByUserNetId.find(userNetId);
	assert(iter != this->mPhysicsControllerByUserNetId.end());	
	this->mPhysicsControllerByUserNetId.erase(iter);
}

void World::update(f32 deltaTime)
{
	this->mTimeSinceLastSimulate += deltaTime;
	if (this->mTimeSinceLastSimulate >= this->mSimulationFrequency)
	{
		this->mTimeSinceLastSimulate -= this->mSimulationFrequency;

		this->mpSystemIntegratePlayerPhysics->tick(this->mSimulationFrequency);
		this->mpSystemPhysicsIntegration->tick(this->mSimulationFrequency);
		this->mOverworld.mpScene->simulate(this->mSimulationFrequency);

		this->onSimulate.broadcast(deltaTime);
	}
}
