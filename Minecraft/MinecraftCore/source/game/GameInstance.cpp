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
#include "ecs/view/ViewPlayerInputMovement.hpp"
#include "ecs/view/ViewPlayerCamera.hpp"
#include "ecs/view/ViewRenderedMesh.hpp"
#include "ecs/view/ViewPhysicalDynamics.hpp"
#include "ecs/system/SystemMovePlayerByInput.hpp"
#include "game/GameClient.hpp"
#include "game/GameWorldLogic.hpp"
#include "input/InputCore.hpp"
#include "input/Queue.hpp"
#include "math/Vector.hpp"
#include "math/Matrix.hpp"
#include "network/NetworkCore.hpp"
#include "network/NetworkPacketChatMessage.hpp"
#include "network/NetworkPacketSetName.hpp"
#include "utility/StringUtils.hpp"
#include "ui/TextLogMenu.hpp"

#include <chrono>

using namespace game;

logging::Logger GAME_LOG = DeclareLog("Game");

std::shared_ptr<Game> Game::gpInstance = nullptr;

std::shared_ptr<Game> Game::Create(int argc, char *argv[])
{
	assert(!Game::gpInstance);
	Game::gpInstance = std::make_shared<Game>(argc, argv);
	return Game::Get();
}

std::shared_ptr<Game> Game::Get()
{
	return Game::gpInstance;
}

void Game::Destroy()
{
	assert(Game::gpInstance && Game::gpInstance.use_count() == 1);
	Game::gpInstance.reset();

	if (engine::Engine::Get())
	{
		engine::Engine::Destroy();
	}
}

Game::Game(int argc, char *argv[]) : mbHasLocalUserNetId(false)
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
	auto pEngine = engine::Engine::Create(memoryChunkSizes);
	this->initializeAssetTypes();

	pEngine->networkInterface().packetTypes()
		.addType<network::packet::ChatMessage>()
		.addType<network::packet::SetName>()
		;

	this->mServerSettings.readFromDisk();
	this->mUserSettings.readFromDisk();
	if (args.find("server") != args.end())
	{
		this->mNetMode = network::EType::eServer;
		this->startDedicatedServer();
	}
	else
	{
		this->mNetMode = network::EType::eClient;
		//this->mpWorldLogic = std::make_shared<game::WorldLogic>();
		this->mpClient = std::make_shared<game::Client>();
	}

	this->registerCommands();
}

Game::~Game()
{
}

void Game::registerCommands()
{
	if (!this->mNetMode.includes(network::EType::eClient)) return;
	auto registry = engine::Engine::Get()->commands();
	registry->add(
		command::Signature("setName")
		.pushArgType<std::string>()
		.bind([&](command::Signature const& cmd)
		{
			auto name = cmd.get<std::string>(0);
			this->mUserSettings.setName(name).writeToDisk();
			if (engine::Engine::Get()->networkInterface().hasConnection())
			{
				network::packet::SetName::create()->setName(name).sendToServer();
			}
		})
	);
	registry->add(
		command::Signature("id")
		.bind([&](command::Signature const& cmd)
		{
			this->mProjectLog.log(LOG_INFO, "Name: %s", this->mUserSettings.name().c_str());
		})
	);
	registry->add(
		command::Signature("listUsers")
		.bind([&](command::Signature const& cmd)
		{
			for (auto const& [netId, user] : this->mConnectedUsers)
			{
				this->client()->chat()->addToLog(user.name);
			}
		})
	);
#pragma region Dedicated Servers
	registry->add(
		command::Signature("connect")
		.pushArgType<network::Address>()
		.bind([&](command::Signature const& cmd)
		{
			this->startDedicatedClient(cmd.get<network::Address>(0));
		})
	);
	registry->add(
		command::Signature("disconnect")
		.bind([&](command::Signature const& cmd)
		{
			engine::Engine::Get()->networkInterface().stop();
		})
	);
#pragma endregion
#pragma region Integrated Servers / ClientOnTopOfServer
	registry->add(
		command::Signature("startHost")
		.bind([&](command::Signature const& cmd)
		{
			this->mNetMode |= network::EType::eServer;
			this->startIntegratedClientServer();
		})
	);
	registry->add(
		command::Signature("stopHost")
		.bind([&](command::Signature const& cmd)
		{
			this->mNetMode.remove(network::EType::eServer);
			engine::Engine::Get()->networkInterface().stop();
		})
	);
#pragma endregion

}

void Game::startDedicatedServer()
{
	auto& networkInterface = engine::Engine::Get()->networkInterface();
	networkInterface.onConnectionEstablished.bind(
		[&](network::Interface *pInterface, ui32 connection, ui32 netId)
		{
			this->addConnectedUser(netId);
			for (auto const& anyNetId : pInterface->connectedClientNetIds())
			{
				if (anyNetId == netId) continue;
				auto const& existingUser = this->findConnectedUser(anyNetId);
				network::packet::SetName::create()
					->setNetId(anyNetId)
					.setName(existingUser.name)
					.sendTo(netId);
			}
		}
	);
	networkInterface.onConnectionClosed.bind(
		[&](network::Interface *pInterface, ui32 connection, ui32 netId)
		{
			auto const& user = this->findConnectedUser(netId);
			network::packet::ChatMessage::broadcastServerMessage(
				utility::formatStr("%s has left the server.", user.name.c_str())
			);
			this->removeConnectedUser(netId);
		}
	);
	networkInterface
		.setType(network::EType::eServer)
		.setAddress(network::Address().setPort(this->mServerSettings.port()));
}

void Game::startDedicatedClient(network::Address const& serverAddress)
{
	auto& networkInterface = engine::Engine::Get()->networkInterface();
	networkInterface.onConnectionEstablished.bind([&](network::Interface *pInterface, ui32 connection, ui32 netId)
	{
		network::packet::SetName::create()->setName(this->mUserSettings.name()).sendToServer();
	});
	networkInterface.onNetIdReceived.bind(
		[&](network::Interface *pInterface, ui32 netId) { this->setLocalUserNetId(netId); }
	);
	networkInterface.onClientPeerStatusChanged.bind(
		[&](network::Interface *pInterface, ui32 netId, network::EClientStatus status)
		{
			if (status == network::EClientStatus::eConnected) this->addConnectedUser(netId);
			else this->removeConnectedUser(netId);
		}
	);
	networkInterface.setType(network::EType::eClient).setAddress(serverAddress).start();
}

void Game::startIntegratedClientServer()
{
	auto& networkInterface = engine::Engine::Get()->networkInterface();

	networkInterface
		.setType(network::EType::eServer)
		.setAddress(network::Address().setPort(this->mServerSettings.port()));

	// Set up the local client


	networkInterface.start();
}

void Game::setLocalUserNetId(ui32 netId)
{
	assert(!this->mbHasLocalUserNetId);
	this->mbHasLocalUserNetId = true;
	this->mLocalUserNetId = netId;
	this->addConnectedUser(netId);
}

game::UserIdentity& Game::localUser()
{
	assert(this->mbHasLocalUserNetId);
	return this->findConnectedUser(this->mLocalUserNetId);
}

void Game::addConnectedUser(ui32 netId)
{
	auto id = game::UserIdentity();
	id.netId = netId;
	id.name = "";
	this->mConnectedUsers.insert(std::make_pair(netId, id));
}

game::UserIdentity& Game::findConnectedUser(ui32 netId)
{
	auto iter = this->mConnectedUsers.find(netId);
	assert(iter != this->mConnectedUsers.end());
	return iter->second;
}

void Game::removeConnectedUser(ui32 netId)
{
	auto iter = this->mConnectedUsers.find(netId);
	assert(iter != this->mConnectedUsers.end());
	this->mConnectedUsers.erase(iter);
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
	ecs->views().registerType<ecs::view::PlayerInputMovement>();
	ecs->views().registerType<ecs::view::PlayerCamera>();
	ecs->views().registerType<ecs::view::RenderedMesh>();
	ecs->views().registerType<ecs::view::PhysicalDynamics>();
}

void Game::openProject()
{
	auto pEngine = engine::Engine::Get();
	auto assetManager = pEngine->getAssetManager();
	
	auto projectPath = std::filesystem::absolute("Minecraft.te-project");
	auto projectAssetPath = asset::AssetPath("project", projectPath, true);
	assetManager->addScannedAsset(projectAssetPath, projectPath, asset::EAssetSerialization::Binary);
	
	auto project = asset::TypedAssetPath<asset::Project>(projectAssetPath).load(asset::EAssetSerialization::Binary);
	pEngine->setProject(project);
	this->mProjectLog = DeclareLog(project->getDisplayName().c_str());

	assetManager->scanAssetDirectory(project->getAssetDirectory(), asset::EAssetSerialization::Binary);
}

void Game::init()
{
	if (auto networkInitError = network::init())
	{
		this->mProjectLog.log(LOG_ERR, "Failed to initialize network: %s", networkInitError->c_str());
		return;
	}

	if (this->mpClient)
	{
		this->mpClient->init();
	}
	if (this->mpWorldLogic)
	{
		this->mpWorldLogic->init();
	}
	
	//this->bindInput();

	auto& netInterface = engine::Engine::Get()->networkInterface();
	if (netInterface.type() == network::EType::eServer)
	{
		netInterface.start();
	}
}

void Game::uninit()
{
	engine::Engine::Get()->networkInterface().stop();

	//this->unbindInput();

	if (this->mpWorldLogic)
	{
		this->mpWorldLogic->uninit();
		this->mpWorldLogic.reset();
	}
	if (this->mpClient)
	{
		this->mpClient->uninit();
		this->mpClient.reset();
	}

	network::uninit();
}

/*
void Game::createLocalPlayer()
{
	auto pEngine = engine::Engine::Get();
	auto& ecs = pEngine->getECS();
	auto& components = ecs.components();
	auto& views = ecs.views();

	this->mpEntityLocalPlayer = ecs.entities().create();

	auto position = this->mpWorld->makeSpawnLocation();

	// Add Transform
	{
		auto transform = components.create<ecs::component::CoordinateTransform>();
		transform->setPosition(position);
		transform->setOrientation(math::Vector3unitY, 0); // force the camera to face forward (-Z)
		this->mpEntityLocalPlayer->addComponent(transform);
	}

	// Add PlayerInput support for moving the entity around
	{
		// View and Component can be added in any order.
		// This order (transform, view, player-input) is used to test the any-order functionality

		this->mpEntityLocalPlayer->addView(views.create<ecs::view::PlayerInputMovement>());

		auto input = components.create<ecs::component::PlayerInput>();
		input->subscribeToQueue();
		this->mpEntityLocalPlayer->addComponent(input);
	}

	if (this->mpClient)
	{
		// Camera Perspective support for rendering from the entity
		// Required by `view::CameraPerspective`
		auto cameraPOV = components.create<ecs::component::CameraPOV>();
		cameraPOV->setFOV(27.0f); // 45.0f horizontal FOV
		this->mpEntityLocalPlayer->addComponent(cameraPOV);

		// This view enables the `UpdateCameraPerspective` system to run
		this->mpEntityLocalPlayer->addView(views.create<ecs::view::PlayerCamera>());
	
		// This view enables the `RenderEntities` system to run
		this->mpEntityLocalPlayer->addView(views.create<ecs::view::RenderedMesh>());

		// Required by `view::RenderedMesh`
		auto mesh = components.create<ecs::component::RenderMesh>();
		mesh->setModel(asset::TypedAssetPath<asset::Model>::Create(
			"assets/models/DefaultHumanoid/DefaultHumanoid.te-asset"
		));
		mesh->setTextureId("model:DefaultHumanoid");
		this->mpEntityLocalPlayer->addComponent(mesh);
	}

	{
		auto component = components.create<ecs::component::PhysicsController>();
		auto extents = math::Vector3{ 0.4f, 0.9f, 0.4f };
		component
			->setIsAffectedByGravity(false)
			.controller()
				.setScene(this->mpSceneOverworld)
				.setAsBox(extents)
				.setCenterPosition(position.toGlobal() + math::Vector<f64, 3>({ 0, extents.y(), 0 }))
				.setMaterial(this->mpPlayerPhysicsMaterial.get())
				.create();
		this->mpEntityLocalPlayer->addComponent(component);
	}
}

std::shared_ptr<ecs::Entity> Game::localPlayer() { return this->mpEntityLocalPlayer; }
//*/

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
	//if (this->mpWorldLogic) this->mpWorldLogic->update(deltaTime);
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
