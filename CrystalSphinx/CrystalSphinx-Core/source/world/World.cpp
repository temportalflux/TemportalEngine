#include "world/World.hpp"

#include "Engine.hpp"
#include "asset/BlockType.hpp"
#include "ecs/system/SystemPhysicsIntegration.hpp"
#include "physics/ChunkCollisionManager.hpp"
#include "physics/PhysicsMaterial.hpp"
#include "physics/PhysicsRigidBody.hpp"
#include "physics/PhysicsScene.hpp"
#include "physics/PhysicsShape.hpp"
#include "physics/PhysicsSystem.hpp"
#include "registry/VoxelType.hpp"
#include "saveData/SaveDataRegistry.hpp"
#include "world/WorldTerrain.hpp"

using namespace game;

logging::Logger WORLD_LOG = DeclareLog("World", LOG_INFO);

World::World() : mpSaveInstance(nullptr)
{
}

void World::init(saveData::Instance *saveInstance)
{
	WORLD_LOG.log(LOG_INFO, "Initializing world save %s", saveInstance->name().c_str());
	this->mpSaveInstance = saveInstance;
	this->mSaveData = world::SaveData(this->mpSaveInstance->worldSave());
	this->mSaveData.readFromDisk();

	//this->createVoxelTypeRegistry();

	this->initializePhysics();

	this->mOverworld.id = 0;
	this->createDimension(&this->mOverworld);
}

void World::uninit()
{
	if (this->mpSaveInstance == nullptr) return;
	WORLD_LOG.log(LOG_INFO, "Destroying world");
	this->destroyDimension(&this->mOverworld);
	this->uninitializePhysics();
	this->mpVoxelTypeRegistry.reset();
	this->mpSaveInstance = nullptr;
}

world::SaveData& World::saveData() { return this->mSaveData; }

std::shared_ptr<game::VoxelTypeRegistry> World::voxelTypeRegistry() { return this->mpVoxelTypeRegistry; }

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
	WORLD_LOG.log(
		LOG_INFO, "Creating dimension %s:%u",
		this->mpSaveInstance->name().c_str(), dim->id
	);
	dim->mpScene = std::make_shared<physics::Scene>();
	dim->mpScene->setSystem(this->mpPhysics);
	dim->mpScene->create();

	dim->mpChunkCollisionManager = std::make_shared<physics::ChunkCollisionManager>(
		this->mpPhysics, dim->mpScene
	);

	dim->mpTerrain = std::make_shared<world::Terrain>(this->mSaveData.seed());
	//if (this->mpVoxelInstanceBuffer) this->mpTerrain->addEventListener(this->mpVoxelInstanceBuffer);
	dim->mpTerrain->addEventListener(dim->mpChunkCollisionManager);
}

void World::destroyDimension(Dimension *dim)
{
	WORLD_LOG.log(
		LOG_INFO, "Destroying dimension %s:%u",
		this->mpSaveInstance->name().c_str(), dim->id
	);
	dim->mpTerrain.reset();
	dim->mpChunkCollisionManager.reset();
	dim->mpScene.reset();
}

/*
void World::createWorld()
{
	//this->createLocalPlayer();

	// Specifically for clients which set player movement/camera information
	{
		auto pEngine = engine::Engine::Get();
		this->mpSystemMovePlayerByInput = pEngine->getMainMemory()->make_shared<ecs::system::MovePlayerByInput>();
		pEngine->addTicker(this->mpSystemMovePlayerByInput);
	}
}

void World::createScene()
{
	//this->mpTerrain->loadChunk({ 0, 0, 0 });
	//this->mpTerrain->loadChunk({ 0, 0, -1 });
	//for (i32 x = -1; x <= 1; ++x) for (i32 z = -1; z <= 1; ++z)
	//	this->mpTerrain->loadChunk({ x, 0, z });
}

void World::destroyScene()
{
	//this->mpSystemMovePlayerByInput.reset();
	//this->mpEntityLocalPlayer.reset();
}
//*/

void World::update(f32 deltaTime)
{
	this->mOverworld.mpScene->simulate(deltaTime);
}
