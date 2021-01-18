#include "game/GameInstance.hpp"
#include "game/GameInstance.hpp"

#include "Engine.hpp"
#include "asset/BlockType.hpp"
#include "asset/Project.hpp"
#include "command/CommandRegistry.hpp"
#include "ecs/Core.hpp"
#include "ecs/entity/Entity.hpp"
#include "ecs/component/CoordinateTransform.hpp"
#include "ecs/component/ComponentPlayerInput.hpp"
#include "ecs/component/ComponentCameraPOV.hpp"
#include "ecs/component/ComponentRenderMesh.hpp"
#include "ecs/component/ComponentPhysicsBody.hpp"
#include "ecs/component/ComponentPhysicsController.hpp"
#include "ecs/component/ComponentPlayerPhysics.hpp"
#include "ecs/view/ViewPlayerInputMovement.hpp"
#include "ecs/view/ViewPlayerCamera.hpp"
#include "ecs/view/ViewRenderedMesh.hpp"
#include "ecs/view/ViewPhysicalDynamics.hpp"
#include "ecs/system/SystemMovePlayerByInput.hpp"
#include "game/GameClient.hpp"
#include "game/GameServer.hpp"
#include "input/InputCore.hpp"
#include "input/Queue.hpp"
#include "math/Vector.hpp"
#include "math/Matrix.hpp"
#include "network/NetworkCore.hpp"
#include "network/NetworkInterface.hpp"
#include "network/packet/NetworkPacketLoginWithAuthId.hpp"
#include "network/packet/NetworkPacketAuthenticate.hpp"
#include "network/packet/NetworkPacketChatMessage.hpp"
#include "network/packet/NetworkPacketRequestPublicKey.hpp"
#include "network/packet/NetworkPacketUpdateUserInfo.hpp"
#include "utility/StringUtils.hpp"
#include "ui/TextLogMenu.hpp"
#include "world/World.hpp"
#include "world/WorldSaved.hpp"
#include "world/WorldSaveData.hpp"

#include <chrono>

using namespace game;

logging::Logger GAME_LOG = DeclareLog("Game", LOG_INFO);

std::shared_ptr<Game> Singleton<Game, int, char*[]>::gpInstance = nullptr;

Game::Game(int argc, char *argv[])
	: mSaveDataRegistry(std::filesystem::current_path() / "saves")
{
	uSize totalMem = 0;
	auto args = utility::parseArguments(argc, argv);
	
	GAME_LOG.log(LOG_INFO, "Initializing with args:");
	for (auto iter = args.begin(); iter != args.end(); ++iter)
	{
		GAME_LOG.log(
			LOG_INFO, "  %s = %s",
			iter->first.c_str(), iter->second ? iter->second->c_str() : "true"
		);
	}

	auto memoryChunkSizes = utility::parseArgumentInts(args, "memory-", totalMem);
	engine::Engine::Create(memoryChunkSizes);
	this->initializeAssetTypes();

	this->mSaveDataRegistry.scan();
	this->registerCommands();

	auto* networkInterface = Game::networkInterface();
	networkInterface->packetTypes()
		.addType<network::packet::LoginWithAuthId>()
		.addType<network::packet::RequestPublicKey>()
		.addType<network::packet::Authenticate>()
		.addType<network::packet::UpdateUserInfo>()
		.addType<network::packet::ChatMessage>()
		;
	networkInterface->onConnectionEstablished.bind(std::bind(
		&Game::onNetworkConnectionOpened, this,
		std::placeholders::_1, std::placeholders::_2, std::placeholders::_3
	));
	
	this->mServerSaveId = std::nullopt;
	auto serverArg = args.find("server");
	if (serverArg != args.end())
	{
		assert(serverArg->second);
		this->mServerSaveId = *serverArg->second;
	}
	else
	{
		this->mpClient = std::make_shared<game::Client>();
	}
}

Game::~Game()
{
	this->mpServer.reset();
	if (engine::Engine::Get())
	{
		engine::Engine::Destroy();
	}
}

void Game::registerCommands()
{
}

network::Interface* Game::networkInterface()
{
	return &engine::Engine::Get()->networkInterface();
}

std::shared_ptr<game::Server> Game::server() { return this->mpServer; }
std::shared_ptr<game::Client> Game::client() { return this->mpClient; }
saveData::Registry& Game::saveData() { return this->mSaveDataRegistry; }
std::shared_ptr<world::World> Game::world() { return this->mpWorld; }

void Game::destroyServer()
{
	if (this->mpServer)
	{
		this->mpServer.reset();
	}
}

void Game::destroyWorld()
{
	if (this->mpWorld)
	{
		this->mpWorld->uninit();
		this->mpWorld.reset();
	}
}

void Game::setupNetworkServer(utility::Flags<network::EType> flags, saveData::Instance *saveInstance)
{
	auto const bIsDedicated = flags == network::EType::eServer;
	this->mpServer = std::make_shared<game::Server>();
	this->server()->loadFrom(saveInstance);
	this->mpServer->setupNetwork(flags);
}

void Game::onNetworkConnectionOpened(network::Interface *pInterface, ui32 connection, ui32 netId)
{
	if (pInterface->type().includes(network::EType::eServer))
	{
		if (connection == pInterface->connection() && pInterface->type().includes(network::EType::eClient))
		{
			this->client()->onLocalServerConnectionOpened(pInterface, connection, netId);
		}
		this->server()->onNetworkConnectionOpened(pInterface, connection, netId);
	}
	// this, a dedicated client, has joined a server. tell the server about our name
	else if (pInterface->type() == network::EType::eClient)
	{
		this->client()->onNetworkConnectionOpened(pInterface, connection, netId);
	}
}

std::shared_ptr<asset::AssetManager> Game::assetManager()
{
	return engine::Engine::Get()->getAssetManager();
}

void Game::initializeAssetTypes()
{
	auto assetManager = Game::assetManager();
	assetManager->queryAssetTypes();
	assetManager->registerType<asset::BlockType>();
}

bool Game::initializeSystems()
{
	auto pEngine = engine::Engine::Get();
	if (!pEngine->initializeDependencies()) return false;
	pEngine->ECSRegisterTypesEvent.bind(std::bind(&Game::registerECSTypes, this, std::placeholders::_1));
	pEngine->initializeECS();
	return true;
}

void Game::registerECSTypes(ecs::Core *ecs)
{
	ecs->components().registerType<ecs::component::CoordinateTransform>("CoordinateTransform");
	ecs->components().registerType<ecs::component::PlayerInput>("PlayerInput");
	ecs->components().registerType<ecs::component::CameraPOV>("CameraPOV");
	ecs->components().registerType<ecs::component::RenderMesh>("RenderMesh");
	ecs->components().registerType<ecs::component::PhysicsBody>("PhysicsBody");
	ecs->components().registerType<ecs::component::PhysicsController>("PhysicsController");
	ecs->components().registerType<ecs::component::PlayerPhysics>("PlayerPhysics");
	ecs->views().registerType<ecs::view::PlayerInputMovement>("PlayerInputMovement");
	ecs->views().registerType<ecs::view::PlayerCamera>("PlayerCamera");
	ecs->views().registerType<ecs::view::RenderedMesh>("RenderedMesh");
	ecs->views().registerType<ecs::view::PhysicalDynamics>("PhysicalDynamics");
}

void Game::openProject()
{
	auto pEngine = engine::Engine::Get();
	auto assetManager = pEngine->getAssetManager();
	
	auto projectPath = std::filesystem::absolute("CrystalSphinx.te-project");
	auto projectAssetPath = asset::AssetPath("project", projectPath, true);
	assetManager->addScannedAsset(projectAssetPath, projectPath, asset::EAssetSerialization::Binary);
	
	auto project = asset::TypedAssetPath<asset::Project>(projectAssetPath).load(asset::EAssetSerialization::Binary);
	pEngine->setProject(project);
	this->mProjectLog = DeclareLog(project->getDisplayName().c_str(), LOG_INFO);

	assetManager->scanAssetDirectory(project->getAssetDirectory(), asset::EAssetSerialization::Binary);
}

void Game::init()
{
	if (auto networkInitError = network::init())
	{
		this->mProjectLog.log(LOG_ERR, "Failed to initialize network: %s", networkInitError->c_str());
		return;
	}

	if (this->mServerSaveId)
	{
		if (!this->mSaveDataRegistry.has(*this->mServerSaveId))
		{
			this->mSaveDataRegistry.create(*this->mServerSaveId);
		}
		auto* saveInstance = &this->mSaveDataRegistry.get(*this->mServerSaveId);
		this->createWorld<world::WorldSaved>(saveInstance);
		this->setupNetworkServer(network::EType::eServer, saveInstance);
	}
	if (this->mpServer) this->mpServer->init();
	if (this->mpWorld) this->mpWorld->init();
	if (this->mpClient) this->mpClient->init();
	
	//this->bindInput();

	auto& netInterface = *Game::networkInterface();
	if (netInterface.type().includes(network::EType::eServer))
	{
		netInterface.start();
	}
}

void Game::uninit()
{
	Game::networkInterface()->stop();

	//this->unbindInput();

	if (this->mpWorld)
	{
		this->mpWorld->uninit();
		this->mpWorld.reset();
	}
	if (this->mpClient)
	{
		this->mpClient->uninit();
		this->mpClient.reset();
	}

	network::uninit();
}

/*
void Game::bindInput()
{
	auto pInput = engine::Engine::Get()->getInputQueue();
	pInput->OnInputEvent.bind(
		input::EInputType::KEY, this->weak_from_this(),
		std::bind(&Game::onInputKey, this, std::placeholders::_1)
	);
}

void Game::unbindInput()
{
	auto pInput = engine::Engine::Get()->getInputQueue();
	pInput->OnInputEvent.unbind(input::EInputType::KEY, this->weak_from_this());
}
//*/

void Game::run()
{
	OPTICK_THREAD("MainThread");
	auto pEngine = engine::Engine::Get();

	pEngine->start();
	auto prevTime = std::chrono::high_resolution_clock::now();
	f32 deltaTime = 0.0f;
	while (pEngine->isActive())
	{
		this->update(deltaTime);
		auto nextTime = std::chrono::high_resolution_clock::now();
		deltaTime = std::chrono::duration<f32, std::chrono::seconds::period>(nextTime - prevTime).count();
		prevTime = nextTime;		
	}
	pEngine->joinThreads();
}

void Game::update(f32 deltaTime)
{
	OPTICK_EVENT();

	//this->mpWorld->handleDirtyCoordinates();
	engine::Engine::Get()->update(deltaTime);
	if (this->mpWorld) this->mpWorld->update(deltaTime);
}

/*
void Game::onInputKey(input::Event const& evt)
{
	if (evt.inputKey.action != input::EAction::RELEASE) return;
	if (input::isTextInputActive()) return;
	if (evt.inputKey.key == input::EKey::NUM_1)
	{
		this->mProjectLog.log(LOG_INFO, "Regenerate");
		this->mpWorld->reloadChunk({ 0, 0, 0 });
	}
	if (evt.inputKey.key == input::EKey::F6)
	{
		if (this->mpChunkBoundaryRenderer->isBoundaryEnabled(graphics::ChunkBoundaryType::eSideGrid))
		{
			this->mpChunkBoundaryRenderer->setIsBoundaryEnabled(graphics::ChunkBoundaryType::eColumn, false);
			this->mpChunkBoundaryRenderer->setIsBoundaryEnabled(graphics::ChunkBoundaryType::eCube, false);
			this->mpChunkBoundaryRenderer->setIsBoundaryEnabled(graphics::ChunkBoundaryType::eSideGrid, false);
		}
		else if (this->mpChunkBoundaryRenderer->isBoundaryEnabled(graphics::ChunkBoundaryType::eCube))
		{
			this->mpChunkBoundaryRenderer->setIsBoundaryEnabled(graphics::ChunkBoundaryType::eSideGrid, true);
		}
		else if (this->mpChunkBoundaryRenderer->isBoundaryEnabled(graphics::ChunkBoundaryType::eColumn))
		{
			this->mpChunkBoundaryRenderer->setIsBoundaryEnabled(graphics::ChunkBoundaryType::eCube, true);
		}
		else
		{
			this->mpChunkBoundaryRenderer->setIsBoundaryEnabled(graphics::ChunkBoundaryType::eColumn, true);
		}
	}
}
//*/
