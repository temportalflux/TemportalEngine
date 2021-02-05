#include "game/GameClient.hpp"

#include "Engine.hpp"
#include "asset/Font.hpp"
#include "asset/ModelAsset.hpp"
#include "asset/RenderPassAsset.hpp"
#include "asset/PipelineAsset.hpp"
#include "asset/Texture.hpp"
#include "asset/TextureSampler.hpp"
#include "asset/MinecraftAssetStatics.hpp"
#include "evcs/component/CoordinateTransform.hpp"
#include "evcs/component/ComponentCameraPOV.hpp"
#include "evcs/component/ComponentRenderMesh.hpp"
#include "evcs/view/ViewPlayerInputMovement.hpp"
#include "evcs/view/ViewPlayerCamera.hpp"
#include "evcs/view/ViewRenderedMesh.hpp"
#include "evcs/view/ViewPhysicalDynamics.hpp"
#include "evcs/system/SystemUpdateCameraPerspective.hpp"
#include "evcs/system/SystemRenderEntities.hpp"
#include "evcs/system/SystemMovePlayerByInput.hpp"
#include "game/GameInstance.hpp"
#include "game/GameServer.hpp"
#include "input/InputCore.hpp"
#include "input/Queue.hpp"
#include "network/packet/NetworkPacketDisplacePlayer.hpp"
#include "network/packet/NetworkPacketLoginWithAuthId.hpp"
#include "network/packet/NetworkPacketUpdateUserInfo.hpp"
#include "render/EntityInstanceBuffer.hpp"
#include "render/ImmediateRenderSystem.hpp"
#include "render/ModelSimple.hpp"
#include "render/TextureRegistry.hpp"
#include "render/line/SimpleLineRenderer.hpp"
#include "render/line/ChunkBoundaryRenderer.hpp"
#include "render/model/SkinnedModelManager.hpp"
#include "render/ui/UIRenderer.hpp"
#include "render/voxel/BlockInstanceMap.hpp"
#include "render/voxel/VoxelGridRenderer.hpp"
#include "render/voxel/VoxelModelManager.hpp"
#include "resource/ResourceManager.hpp"
#include "ui/DebugHUD.hpp"
#include "ui/TextLogMenu.hpp"
#include "ui/UIWidgets.hpp"
#include "utility/Colors.hpp"
#include "window/Window.hpp"
#include "world/World.hpp"
#include "world/WorldSaved.hpp"
#include "world/WorldReplicated.hpp"

using namespace game;

static auto CLIENT_LOG = DeclareLog("Client", LOG_INFO);

Client::Client()
	: Session()
	, mLocalUserNetId(std::nullopt)
{
	this->registerCommands();
	this->userRegistry().scan("users");
	this->settings().readFromDisk();
}

std::shared_ptr<Window> Client::getWindow() { return this->mpWindow; }
std::shared_ptr<graphics::ImmediateRenderSystem> Client::renderer() { return this->mpRenderer; }
std::shared_ptr<graphics::SkinnedModelManager> Client::modelManager() { return this->mpSkinnedModelManager; }
std::shared_ptr<graphics::EntityInstanceBuffer> Client::entityInstances() { return this->mpEntityInstanceBuffer; }
std::shared_ptr<graphics::TextureRegistry> Client::textureRegistry() { return this->mpTextureRegistry; }
std::shared_ptr<ui::FontOwner> Client::uiFontOwner() { return this->mpUIRenderer; }
std::shared_ptr<ui::TextLogMenu> Client::chat() { return this->mpMenuTextLog; }
game::ClientSettings& Client::settings() { return this->mClientSettings; }

logging::Logger& Client::logger() { return CLIENT_LOG; }

void Client::init()
{
	Session::init();

	auto* networkInterface = Game::networkInterface();
	networkInterface->onConnectionEstablished.bind(this->weak_from_this(), std::bind(
		&game::Client::sendAuthenticationId, this,
		std::placeholders::_1, std::placeholders::_2
	));
	networkInterface->OnClientAuthenticatedOnClient.bind(std::bind(
		&game::Client::onClientAuthenticated, this,
		std::placeholders::_1, std::placeholders::_2
	));
	networkInterface->onClientPeerStatusChanged.bind(std::bind(
		&game::Client::onClientPeerStatusChanged, this,
		std::placeholders::_1, std::placeholders::_2, std::placeholders::_3
	));
	networkInterface->OnDedicatedClientDisconnected.bind(this->weak_from_this(), std::bind(
		&game::Client::onDedicatedClientDisconnected, this,
		std::placeholders::_1, std::placeholders::_2
	));
	networkInterface->onNetworkStopped.bind(this->weak_from_this(), std::bind(
		&Client::onNetworkStopped, this, std::placeholders::_1
	));

	evcs::Core::Get()->onOwnershipChanged.bind(this->weak_from_this(), std::bind(
		&Client::onEVCSOwnershipChanged, this,
		std::placeholders::_1, std::placeholders::_2, std::placeholders::_3
	));

	if (!this->scanResourcePacks()) return;
	if (!this->initializeGraphics()) return;
	if (!this->createWindow()) return;
	this->createRenderers();
	this->mpResourcePackManager->loadPack("Default", 0).commitChanges();
	this->mpSystemRenderEntities->createLocalPlayerDescriptor();
	this->mpRenderer->createMutableUniforms();
	this->mpRenderer->createRenderChain();
	this->mpRenderer->finalizeInitialization();

	// Specifically for clients which set player movement/camera information
	{
		auto pEngine = engine::Engine::Get();
		this->mpSystemMovePlayerByInput = pEngine->getMainMemory()->make_shared<evcs::system::MovePlayerByInput>();
		pEngine->addTicker(this->mpSystemMovePlayerByInput);
		this->mpSystemMovePlayerByInput->subscribeToQueue();
	}

	this->bindInput();
}

void Client::uninit()
{
	this->unbindInput();
	this->destroyRenderers();
	this->mpSystemMovePlayerByInput.reset();
	this->mpResourcePackManager.reset();
	this->destroyWindow();
	Session::uninit();
}

void Client::registerCommands()
{
	ADD_CMD(
		CMD_SIGNATURE("setRes", Client, this, exec_setResolution)
		.pushArgType<ui32>().pushArgType<ui32>()
	);
	ADD_CMD(CMD_SIGNATURE("setResRatio", Client, this, exec_setResolutionRatio).pushArgType<ui32>());
	ADD_CMD(CMD_SIGNATURE("setDPI", Client, this, exec_setDPI).pushArgType<ui32>());
	
	ADD_CMD(CMD_SIGNATURE("listIds", Client, this, exec_printAccountList));
	ADD_CMD(CMD_SIGNATURE("createId", Client, this, exec_createAccount).pushArgType<std::string>());
	ADD_CMD(CMD_SIGNATURE("setId", Client, this, exec_setAccount).pushArgType<ui32>());
	ADD_CMD(CMD_SIGNATURE("printId", Client, this, exec_printAccount));
	ADD_CMD(CMD_SIGNATURE("setName", Client, this, exec_setAccountName).pushArgType<std::string>());

	ADD_CMD(CMD_SIGNATURE("openSave", Client, this, exec_openSave).pushArgType<std::string>());
	ADD_CMD(CMD_SIGNATURE("closeSave", Client, this, exec_closeSave));

	ADD_CMD(CMD_SIGNATURE("join", Client, this, exec_joinServer).pushArgType<network::Address>());
	ADD_CMD(CMD_SIGNATURE("joinLocal", Client, this, exec_joinServerLocal));
	ADD_CMD(CMD_SIGNATURE("leave", Client, this, exec_leaveServer));
	
	ADD_CMD(CMD_SIGNATURE("startHost", Client, this, exec_startHostingServer));
	ADD_CMD(CMD_SIGNATURE("stopHost", Client, this, exec_stopHostingServer));

	ADD_CMD(CMD_SIGNATURE("listUsers", Client, this, exec_printConnectedUsers));
}

void Client::exec_setResolution(command::Signature const& cmd)
{
	math::Vector2UInt res = { cmd.get<ui32>(0), cmd.get<ui32>(1) };
	this->settings().setResolution(res).writeToDisk();
	this->getWindow()->setSize(res);
}

void Client::exec_setResolutionRatio(command::Signature const& cmd)
{
	math::Vector2UInt res = math::Vector2UInt({ 16, 9 }) * cmd.get<ui32>(0);
	this->settings().setResolution(res).writeToDisk();
	this->getWindow()->setSize(res);
}

void Client::exec_setDPI(command::Signature const& cmd)
{
	this->settings().setDPI(cmd.get<ui32>(0)).writeToDisk();
	this->renderer()->setDPI(cmd.get<ui32>(0));
}

void Client::exec_printAccountList(command::Signature const& cmd)
{
	std::stringstream ss;
	auto const userCount = this->userRegistry().getUserCount();
	if (userCount == 0) ss << "There are no users";
	for (uIndex i = 0; i < userCount; ++i)
	{
		auto const& userId = this->userRegistry().getId(i);
		auto userInfo = this->userRegistry().loadInfo(userId);
		if (i > 0) ss << '\n';
		ss << i << ". " << userInfo.name().c_str();
	}
	this->chat()->addToLog(ss.str());
}

void Client::exec_createAccount(command::Signature const& cmd)
{
	auto const& userId = this->userRegistry().createUser();
	auto userInfo = this->userRegistry().loadInfo(userId);
	userInfo.setName(cmd.get<std::string>(0)).writeToDisk();
}

void Client::exec_setAccount(command::Signature const& cmd)
{
	auto userIdx = cmd.get<ui32>(0);
	this->mLocalUserId = std::nullopt;
	if (userIdx < this->userRegistry().getUserCount())
	{
		this->mLocalUserId = this->userRegistry().getId(userIdx);
	}
}

void Client::exec_printAccount(command::Signature const& cmd)
{
	std::stringstream ss;
	if (this->hasAccount())
	{
		auto userInfo = this->localUserInfo();
		ss
			<< "Id: " << this->localUserId().toString().c_str()
			<< '\n'
			<< "Name: " << userInfo.name().c_str()
			;
	}
	else
	{
		ss << "You have not selected an account.";
	}
	this->chat()->addToLog(ss.str());
}

void Client::exec_setAccountName(command::Signature const& cmd)
{
	if (!this->hasAccount())
	{
		this->chat()->addToLog("You have not selected an account.");
		return;
	}
	auto name = cmd.get<std::string>(0);
	auto userInfo = this->localUserInfo();
	userInfo.setName(name).writeToDisk();
	if (Game::networkInterface()->hasConnection())
	{
		network::packet::UpdateUserInfo::create()->setInfo(userInfo).sendToServer();
	}
}

void Client::exec_openSave(command::Signature const& cmd)
{
	auto pGame = game::Game::Get();
	auto& saveData = pGame->saveData();
	auto const saveName = cmd.get<std::string>(0);
	if (!saveData.has(saveName))
	{
		saveData.create(saveName);
	}

	auto pWorld = game::Game::Get()->createWorld<world::WorldSaved>(&saveData.get(saveName));
	pWorld->onSimulate.bind(this->weak_from_this(), std::bind(&Client::onWorldSimulate, this, std::placeholders::_1));
	pWorld->init();
	pWorld->addTerrainEventListener(0, this->mpVoxelInstanceBuffer);
	pWorld->loadChunk(0, { 0, 0, 0 });
	// TODO: Load player location and rotation from save data
	this->mLocalUserNetId = 0;
	this->mLocalPlayerEntityId = pWorld->createPlayer(
		this->mLocalUserNetId.value(), pWorld->makeSpawnLocation()
	);
}

void Client::exec_closeSave(command::Signature const& cmd)
{
	game::Game::Get()->destroyWorld();
	this->mLocalPlayerEntityId = 0;
}

void Client::exec_joinServer(command::Signature const& cmd)
{
	if (!this->hasAccount())
	{
		this->chat()->addToLog("You have not selected an account.");
		return;
	}

	auto pWorld = game::Game::Get()->createWorld<world::WorldReplicated>();
	pWorld->onSimulate.bind(this->weak_from_this(), std::bind(&Client::onWorldSimulate, this, std::placeholders::_1));
	pWorld->init();
	pWorld->addTerrainEventListener(0, this->mpVoxelInstanceBuffer);

	this->setupNetwork(cmd.get<network::Address>(0));
	Game::networkInterface()->start();
}

void Client::exec_joinServerLocal(command::Signature const& cmd)
{
	if (!this->hasAccount())
	{
		this->chat()->addToLog("You have not selected an account.");
		return;
	}

	auto pWorld = game::Game::Get()->createWorld<world::WorldReplicated>();
	pWorld->onSimulate.bind(this->weak_from_this(), std::bind(&Client::onWorldSimulate, this, std::placeholders::_1));
	pWorld->init();
	pWorld->addTerrainEventListener(0, this->mpVoxelInstanceBuffer);

	auto localAddress = network::Address().setLocalTarget(saveData::ServerSettings().port());
	this->setupNetwork(localAddress);
	Game::networkInterface()->start();
}

void Client::exec_leaveServer(command::Signature const& cmd)
{
	game::Game::Get()->destroyWorld();
	Game::networkInterface()->stop();
}

void Client::exec_startHostingServer(command::Signature const& cmd)
{
	if (!this->hasAccount())
	{
		this->chat()->addToLog("You have not selected an account.");
		return;
	}
	if (!this->world())
	{
		this->chat()->addToLog("There is no world. Try opening a save.");
		return;
	}
	auto pWorld = std::dynamic_pointer_cast<world::WorldSaved>(this->world());
	if (!pWorld)
	{
		CLIENT_LOG.log(LOG_ERR, "Tried hosting a server, but the current world is not saved to disk.");
		return;
	}

	auto pGame = game::Game::Get();
	pGame->setupNetworkServer(
		{ network::EType::eServer, network::EType::eClient },
		pWorld->getSaveInstance()
	);
	pGame->server()->init();
	Game::networkInterface()->start();
}

void Client::exec_stopHostingServer(command::Signature const& cmd)
{
	Game::networkInterface()->stop();
	game::Game::Get()->destroyServer();
}

void Client::exec_printConnectedUsers(command::Signature const& cmd)
{
	auto const& users = this->connectedUsers();
	if (users.size() == 0)
	{
		this->chat()->addToLog("There are no connected users");
		return;
	}
	for (auto const& [netId, userId] : users)
	{
		std::stringstream ss;
		auto& userInfo = this->getConnectedUserInfo(netId);
		ss << netId << ". " << userInfo.name().c_str();
		math::Color color = userInfo.color().toFloat() / 255.0f;
		color.w() = 1.0f;
		this->chat()->addToLog(ss.str(), color); 
	}
}

void Client::setupNetwork(network::Address const& serverAddress)
{
	Game::networkInterface()->setType(network::EType::eClient).setAddress(serverAddress);
}

void Client::sendAuthenticationId(network::Interface *pNetwork, network::ConnectionId connectionId)
{
	// For integrated servers, this will be executed immediately in the same application
	network::packet::LoginWithAuthId::create()
		->setId(this->localUserId())
		.setPublicKey(this->localUserAuthKey().publicData())
		.sendToServer();
}

void Client::onClientAuthenticated(network::Interface *pInteface, network::Identifier netId)
{
	network::logger().log(LOG_INFO, "Network handshake complete, received netId(%u)", netId);

	this->setLocalUserNetId(netId);
	this->addConnectedUser(netId);
	this->getConnectedUserInfo(netId).copyFrom(this->localUserInfo());

	network::packet::UpdateUserInfo::create()->setInfo(this->localUserInfo()).sendToServer();
}

void Client::onDedicatedClientDisconnected(network::Interface *pInteface, network::Identifier invalidNetId)
{
	network::logger().log(LOG_INFO, "Disconnected from network, killing network interface.");
	Game::networkInterface()->stop();
	game::Game::Get()->destroyWorld();
}

void Client::onClientPeerStatusChanged(network::Interface *pInterface, network::Identifier netId, network::EClientStatus status)
{
	switch (status)
	{
		case network::EClientStatus::eConnected:
			this->addConnectedUser(netId);
			break;
		case network::EClientStatus::eDisconnected:
			this->removeConnectedUser(netId);
			break;
		default: assert(false); break;
	}
}

void Client::onNetworkStopped(network::Interface *pInterface)
{
	this->clearConnectedUsers();
}

bool Client::hasAccount() const
{
	return this->mLocalUserId.has_value();
}

utility::Guid const& Client::localUserId() const
{
	return this->mLocalUserId.value();
}

crypto::RSAKey Client::localUserAuthKey() const
{
	return this->userRegistry().loadKey(this->localUserId());
}

game::UserInfo Client::localUserInfo() const
{
	return this->userRegistry().loadInfo(this->localUserId());
}

void Client::setLocalUserNetId(network::Identifier netId)
{
	this->mLocalUserNetId = netId;
}

network::Identifier Client::localUserNetId() const
{
	assert(this->mLocalUserNetId);
	return *this->mLocalUserNetId;
}

bool Client::hasLocalEntity() const
{
	return this->mLocalPlayerEntityId.has_value();
}

evcs::Identifier Client::localUserEntityId() const
{
	return this->mLocalPlayerEntityId.value();
}

void Client::addConnectedUser(network::Identifier netId)
{
	assert(!this->hasConnectedUser(netId));
	Session::addConnectedUser(netId);
	this->mConnectedUserInfo.insert(std::make_pair(netId, game::UserInfo()));
}

void Client::removeConnectedUser(network::Identifier netId)
{
	if (this->hasConnectedUser(netId))
	{
		Session::removeConnectedUser(netId);
		this->mConnectedUserInfo.erase(this->mConnectedUserInfo.find(netId));
	}
}

game::UserInfo& Client::getConnectedUserInfo(network::Identifier netId)
{
	return this->mConnectedUserInfo.find(netId)->second;
}

void Client::onEVCSOwnershipChanged(evcs::EType ecsType, evcs::TypeId typeId, evcs::IEVCSObject *pObject)
{
	if (ecsType == evcs::EType::eEntity && pObject->owner())
	{
		auto pEntity = evcs::Core::Get()->entities().get(pObject->id());
		if (pObject->owner() == this->mLocalUserNetId)
		{
			this->mLocalPlayerEntityId = pObject->id();
			this->addPlayerControlParts(pEntity);
			if (game::Game::networkInterface()->type() == network::EType::eClient)
			{
				this->world()->createPlayerController(this->localUserNetId(), pObject->id());
			}
		}
		this->addPlayerDisplayParts(pEntity);
	}
}

void Client::addPlayerControlParts(std::shared_ptr<evcs::Entity> pEntity)
{
	CLIENT_LOG.log(
		LOG_VERBOSE, "Adding player control components to id(%u)/net(%u), owned by net(%u)",
		pEntity->id(), pEntity->netId(), *pEntity->owner()
	);

	auto* ecs = evcs::Core::Get();

	auto* pTransform = pEntity->getComponent<evcs::component::CoordinateTransform>();

	// enables `system::MovePlayerByInput`
	// requires CoordinateTransform
	// requires PhysicsController
	pEntity->addView(ecs->views().create<evcs::view::PlayerInputMovement>());

}

void Client::addPlayerDisplayParts(std::shared_ptr<evcs::Entity> pEntity)
{
	CLIENT_LOG.log(
		LOG_VERBOSE, "Adding player display components to id(%u)/net(%u), owned by net(%u)",
		pEntity->id(), pEntity->netId(), *pEntity->owner()
	);

	auto* ecs = evcs::Core::Get();

	// Camera PointOfView component
	{
		auto cameraPOV = ecs->components().create<evcs::component::CameraPOV>();
		// TODO: Store in local user display settings
		cameraPOV->setFOV(27.0f); // 45.0f horizontal FOV
		pEntity->addComponent(cameraPOV);
	}

	// enables `system::UpdateCameraPerspective`
	// requires CoordinateTransform
	// requires CameraPOV
	// requires RenderMesh
	pEntity->addView(ecs->views().create<evcs::view::PlayerCamera>());

	// enables `system::RenderEntities`
	// requires CoordinateTransform
	// requires RenderMesh
	pEntity->addView(ecs->views().create<evcs::view::RenderedMesh>());

	// RenderMesh component
	{
		auto mesh = ecs->components().create<evcs::component::RenderMesh>();
		mesh->setModel(asset::TypedAssetPath<asset::Model>::Create(
			"assets/models/DefaultHumanoid/DefaultHumanoid.te-asset"
		));
		mesh->setTextureId("model:DefaultHumanoid");
		pEntity->addComponent(mesh);
	}
}

bool Client::initializeGraphics()
{
	return engine::Engine::Get()->setupVulkan();
}

bool Client::createWindow()
{
	auto pEngine = engine::Engine::Get();
	auto resolution = this->settings().resolution();
	this->mpWindow = pEngine->createWindow(
		resolution.x(), resolution.y(),
		pEngine->getProject()->getDisplayName(),
		WindowFlags::RENDER_ON_THREAD
	);
	if (!this->mpWindow) return false;
	pEngine->initializeVulkan(this->mpWindow->querySDLVulkanExtensions());
	this->mpWindow->showCursor(false);
	this->mpWindow->consumeCursor(true);
	return true;
}

void Client::destroyWindow()
{
	if (this->mpWindow)
	{
		engine::Engine::Get()->destroyWindow(this->mpWindow);
		this->mpWindow.reset();
	}
}

bool Client::scanResourcePacks()
{
	this->mpResourcePackManager = std::make_shared<resource::PackManager>();
	this->mpResourcePackManager->scanPacksIn(std::filesystem::absolute("resource-packs"));

	if (!this->mpResourcePackManager->hasPack("Default"))
	{
		CLIENT_LOG.log(LOG_ERR, "Failed to find default resource pack");
		return false;
	}

	return true;
}

void Client::createRenderers()
{
	auto pEngine = engine::Engine::Get();

	this->createGameRenderer();
	this->loadVoxelTypeTextures();

	this->mpTextureRegistry = std::make_shared<graphics::TextureRegistry>(
		this->mpRenderer->getDevice(), &this->mpRenderer->getTransientPool(), &this->mpRenderer->getDescriptorPool()
	);
	this->mpTextureRegistry->registerSampler(asset::SAMPLER_NEAREST_NEIGHBOR);
	this->mpTextureRegistry->setPackSampler(asset::SAMPLER_NEAREST_NEIGHBOR);
	// Invalid texture
	{
		auto size = math::Vector2UInt{ 16, 16 };
		auto pixels = std::vector<ui8>(size.powDim() * 4);
		for (uIndex iPixel = 0; iPixel < size.powDim(); ++iPixel)
		{
			pixels[iPixel * 4 + 0] = 255;
			pixels[iPixel * 4 + 1] = 0;
			pixels[iPixel * 4 + 2] = 255;
			pixels[iPixel * 4 + 3] = 255;
		}
		this->mpTextureRegistry->registerImage("invalid", size, pixels);
		this->mpTextureRegistry->createDescriptor("invalid", asset::SAMPLER_NEAREST_NEIGHBOR);
		this->mpTextureRegistry->setInvalidTextureId("invalid");
	}
	this->mpResourcePackManager->OnResourcesLoadedEvent.bind(this->mpTextureRegistry, this->mpTextureRegistry->onTexturesLoadedEvent());

	this->mpSystemUpdateCameraPerspective = pEngine->getMainMemory()->make_shared<evcs::system::UpdateCameraPerspective>(
		pEngine->getMiscMemory(), this->mpRenderer
	);
	this->mpSystemUpdateCameraPerspective->subscribeToQueue();
	pEngine->addTicker(this->mpSystemUpdateCameraPerspective);

	this->createPipelineRenderers();

	this->mpSkinnedModelManager = std::make_shared<graphics::SkinnedModelManager>(
		this->mpRenderer->getDevice(), &this->mpRenderer->getTransientPool()
	);
	this->mpEntityInstanceBuffer = std::make_shared<graphics::EntityInstanceBuffer>();
	this->mpEntityInstanceBuffer->setDevice(this->mpRenderer->getDevice());
	this->mpEntityInstanceBuffer->create();

	this->mpSystemRenderEntities = std::make_shared<evcs::system::RenderEntities>();
	pEngine->addTicker(this->mpSystemRenderEntities);
	this->mpSystemRenderEntities->setPipeline(asset::TypedAssetPath<asset::Pipeline>::Create(
		"assets/render/entity/RenderEntityPipeline.te-asset"
	));
	this->mpRenderer->addRenderer(this->mpSystemRenderEntities.get());

	this->mpDebugHUD = std::make_shared<ui::DebugHUD>();
	this->mpDebugHUD->addWidgetsToRenderer(this->mpUIRenderer.get());
	pEngine->addTicker(this->mpDebugHUD);

	this->mpMenuTextLog = std::make_shared<ui::TextLogMenu>();
	this->mpMenuTextLog->init(this->mpUIRenderer.get());
}

void Client::createGameRenderer()
{
	this->mpRenderer = std::make_shared<graphics::ImmediateRenderSystem>();
	this->mpRenderer->setDPI(this->settings().dpi());
	engine::Engine::Get()->initializeRenderer(this->mpRenderer.get(), this->mpWindow);
	this->mpWindow->setRenderer(this->mpRenderer.get());
	this->mpRenderer->UpdateWorldGraphicsOnFramePresented.bind(
		std::bind(&Client::updateWorldGraphics, this)
	);

	// TODO: Configure this per project
	this->mpRenderer->setSwapChainInfo(
		graphics::SwapChainInfo()
		.addFormatPreference(vk::Format::eB8G8R8A8Srgb)
		.setColorSpace(vk::ColorSpaceKHR::eSrgbNonlinear)
		.addPresentModePreference(vk::PresentModeKHR::eMailbox)
		.addPresentModePreference(vk::PresentModeKHR::eFifo)
	);

	// TODO: Configure this per image view (render target)
	// TODO: Image views only need the format from the swapchain. They can be created independently of it too.
	this->mpRenderer->setImageViewInfo(
		{
			vk::ImageViewType::e2D,
			{
				vk::ComponentSwizzle::eIdentity,
				vk::ComponentSwizzle::eIdentity,
				vk::ComponentSwizzle::eIdentity,
				vk::ComponentSwizzle::eIdentity
			}
		}
	);

	auto renderPassAssetPath = engine::Engine::Get()->getProject()->getRenderPass();
	this->mpRenderer->setRenderPass(renderPassAssetPath.load(asset::EAssetSerialization::Binary));
}

void Client::loadVoxelTypeTextures()
{
	this->mpVoxelModelManager = std::make_shared<game::VoxelModelManager>(
		this->mpRenderer->getDevice(), &this->mpRenderer->getTransientPool()
	);
	this->mpVoxelModelManager->setSampler(asset::TypedAssetPath<asset::TextureSampler>(asset::SAMPLER_NEAREST_NEIGHBOR));
	// TODO: world is not initialized
	this->mpVoxelModelManager->loadRegistry(this->voxelTypeRegistry());
	this->mpResourcePackManager->OnResourcesLoadedEvent.bind(this->mpVoxelModelManager, this->mpVoxelModelManager->onTexturesLoadedEvent());
}

void Client::createPipelineRenderers()
{
	this->createVoxelGridRenderer();
	this->createWorldAxesRenderer();
	this->createChunkBoundaryRenderer();
	this->createUIRenderer();
}

constexpr ui64 blockCountForRenderDistance(ui8 chunkRenderDistance)
{
	// Computes (2c+1)^3 by filling all values of a 3-dimensional vector with `2c+1` and multiplying the dimensions together
	// If CRD=5, then count=1331
	// If CRD=6, then count=2197
	// If CRD=11, then count=12167
	ui32 const chunkCount = math::Vector<ui32, 3>(2 * chunkRenderDistance + 1).powDim();
	// the amount of blocks in a cube whose side length is CSL
	// If CSL=16, then blocksPerChunk=4096
	ui32 const blocksPerChunk = math::Vector<ui32, 3>(CHUNK_SIDE_LENGTH).powDim();
	// CRD=5  & CSL=16 ->  1331*4096 ->  5,451,776
	// CRD=6  & CSL=16 ->  2197*4096 ->  8,998,912
	// CRD=11 & CSL=16 -> 12167*4096 -> 49,836,032
	ui64 const totalBlockCount = ui64(chunkCount) * ui64(blocksPerChunk);
	// RenderData has 5 vec4, which is 80 bytes (float is 4 bytes, 4 floats is 16 bytes per vec4, 5 vec4s is 16*5=80)
	// CRD=5  & CSL=16 ->  5,451,776 * 80 ->   436,142,080
	// CRD=6  & CSL=16 ->  8,998,912 * 80 ->   719,912,960 (CRD=7 puts this over 1GB)
	// CRD=11 & CSL=16 -> 49,836,032 * 80 -> 3,986,882,560 (CRD=12 puts this over 4GB)
	return totalBlockCount;
}

void Client::createVoxelGridRenderer()
{
	auto const totalBlockCount = blockCountForRenderDistance(6);

	auto voxelTypes = this->voxelTypeRegistry();
	this->mpVoxelInstanceBuffer = std::make_shared<world::BlockInstanceBuffer>(
		totalBlockCount, voxelTypes
	);
	this->mpVoxelInstanceBuffer->setDevice(this->mpRenderer->getDevice());
	this->mpVoxelInstanceBuffer->createBuffer();

	this->mpVoxelGridRenderer = std::make_shared<graphics::VoxelGridRenderer>(
		&this->mpRenderer->getDescriptorPool(),
		std::weak_ptr(this->mpVoxelInstanceBuffer),
		voxelTypes, this->mpVoxelModelManager
	);
	this->mpVoxelGridRenderer->setPipeline(asset::TypedAssetPath<asset::Pipeline>::Create(
		"assets/render/world/VoxelPipeline.te-asset"
	));
	this->mpRenderer->addRenderer(this->mpVoxelGridRenderer.get());
	this->mpVoxelModelManager->OnAtlasesCreatedEvent.bind(this->mpVoxelGridRenderer, this->mpVoxelGridRenderer->onAtlasesCreatedEvent());
}

void Client::createWorldAxesRenderer()
{
	this->mpWorldAxesRenderer = std::make_shared<graphics::SimpleLineRenderer>();
	this->mpWorldAxesRenderer->setPipeline(asset::TypedAssetPath<asset::Pipeline>::Create(
		"assets/render/debug/PerspectiveLinePipeline.te-asset"
	));
	this->mpRenderer->addRenderer(this->mpWorldAxesRenderer.get());
	// Y: 0->1 green up
	this->mpWorldAxesRenderer->addLineSegment({ math::Vector3::ZERO, 0.5f * math::V3_UP, { 0, 1, 0, 1 } });
	// X: 0->1 red right
	this->mpWorldAxesRenderer->addLineSegment({ math::Vector3::ZERO, 0.5f * math::V3_RIGHT, { 1, 0, 0, 1 } });
	// Z: 0->1 blue forward
	this->mpWorldAxesRenderer->addLineSegment({ math::Vector3::ZERO, 0.5f * math::V3_FORWARD, { 0, 0, 1, 1 } });
	this->mpWorldAxesRenderer->createGraphicsBuffers(&this->mpRenderer->getTransientPool());
}

void Client::createChunkBoundaryRenderer()
{
	this->mpChunkBoundaryRenderer = std::make_shared<graphics::ChunkBoundaryRenderer>();
	this->mpChunkBoundaryRenderer->setPipeline(asset::TypedAssetPath<asset::Pipeline>::Create(
		"assets/render/debug/ChunkLinePipeline.te-asset"
	));
	this->mpRenderer->addRenderer(this->mpChunkBoundaryRenderer.get());

	f32 const l = CHUNK_SIDE_LENGTH;
	auto const axisSides = std::vector<f32>({ 0, l });

	// Columns
	{
		f32 const h = CHUNK_SIDE_LENGTH * 16;
		math::Vector4 color = { 0, 1, 0, 1 };
		auto segments = std::vector<graphics::ChunkBoundaryRenderer::LineSegment>();
		segments.push_back({ { 0, 0, 0 }, { 0, h, 0 }, color });
		segments.push_back({ { l, 0, 0 }, { l, h, 0 }, color });
		segments.push_back({ { l, 0, l }, { l, h, l }, color });
		segments.push_back({ { 0, 0, l }, { 0, h, l }, color });
		this->mpChunkBoundaryRenderer->setBoundarySegments(graphics::ChunkBoundaryType::eColumn, segments, true);
	}
	// Cube
	{
		math::Vector4 color = { 1, 0, 0, 1 };
		auto segments = std::vector<graphics::ChunkBoundaryRenderer::LineSegment>();
		for (f32 h : axisSides)
		{
			segments.push_back({ { 0, h, 0 }, { l, h, 0 }, color });
			segments.push_back({ { l, h, 0 }, { l, h, l }, color });
			segments.push_back({ { 0, h, 0 }, { 0, h, l }, color });
			segments.push_back({ { 0, h, l }, { l, h, l }, color });
		}
		this->mpChunkBoundaryRenderer->setBoundarySegments(graphics::ChunkBoundaryType::eCube, segments, false);
	}
	// Side Grid
	{
		math::Vector4 color = { 0, 0, 1, 1 };
		auto segments = std::vector<graphics::ChunkBoundaryRenderer::LineSegment>();

		// Y-Faces (Up/Down)
		for (auto const y : axisSides)
		{
			FOR_CHUNK_SIZE_PLUS(f32, x, 1, 0)
			{
				segments.push_back({ { x, y, 0 }, { x, y, l }, color });
			}
			FOR_CHUNK_SIZE_PLUS(f32, z, 1, 0)
			{
				segments.push_back({ { 0, y, z }, { l, y, z }, color });
			}
		}
		// Z-Faces (Back/Front)
		for (auto const z : axisSides)
		{
			FOR_CHUNK_SIZE_PLUS(f32, x, 1, 0)
			{
				segments.push_back({ { x, 0, z }, { x, l, z }, color });
			}
			FOR_CHUNK_SIZE_PLUS(f32, y, 1, 0)
			{
				segments.push_back({ { 0, y, z }, { l, y, z }, color });
			}
		}
		// X-Faces (Left/Right)
		for (auto const x : axisSides)
		{
			FOR_CHUNK_SIZE_PLUS(f32, y, 1, 0)
			{
				segments.push_back({ { x, y, 0 }, { x, y, l }, color });
			}
			FOR_CHUNK_SIZE_PLUS(f32, z, 1, 0)
			{
				segments.push_back({ { x, 0, z }, { x, l, z }, color });
			}
		}

		this->mpChunkBoundaryRenderer->setBoundarySegments(graphics::ChunkBoundaryType::eSideGrid, segments, false);
	}
	this->mpChunkBoundaryRenderer->createGraphicsBuffers(&this->mpRenderer->getTransientPool());
}

void Client::createUIRenderer()
{
	this->mpUIRenderer = std::make_shared<graphics::UIRenderer>();
	this->mpUIRenderer->setTextPipeline(asset::TypedAssetPath<asset::Pipeline>::Create(
		"assets/render/ui/UIPipeline.te-asset"
	));
	this->mpUIRenderer->setImagePipeline(asset::TypedAssetPath<asset::Pipeline>::Create(
		"assets/render/ui/ImagePipeline.te-asset"
	));
	this->mpUIRenderer->addFont("unispace", asset::TypedAssetPath<asset::Font>::Create(
		"assets/font/Unispace/Unispace.te-asset"
	).load(asset::EAssetSerialization::Binary));
	this->mpRenderer->addRenderer(this->mpUIRenderer.get());

	ui::createResources({
		this->mpRenderer->getDevice(),
		&this->mpRenderer->getTransientPool(),
		&this->mpUIRenderer->imageDescriptorLayout(),
		&this->mpRenderer->getDescriptorPool(),
		&this->mpUIRenderer->imageSampler()
	});
}

void Client::destroyRenderers()
{
	this->mpSystemRenderEntities->destroy();
	this->mpTextureRegistry.reset();
	this->mpEntityInstanceBuffer.reset();
	this->mpSkinnedModelManager.reset();
	this->mpVoxelModelManager.reset();
	if (this->mpVoxelGridRenderer) this->mpVoxelGridRenderer->destroy();
	this->mpWorldAxesRenderer->destroy();
	this->mpChunkBoundaryRenderer->destroy();
	this->mpVoxelInstanceBuffer.reset();
	this->mpMenuTextLog.reset();
	this->mpDebugHUD.reset();

	this->mpSystemUpdateCameraPerspective.reset();
	this->mpUIRenderer->destroyRenderDevices();
	ui::destroyResources();

	this->mpRenderer->invalidate();

	this->mpSystemRenderEntities.reset();
	this->mpVoxelGridRenderer.reset();
	this->mpWorldAxesRenderer.reset();
	this->mpChunkBoundaryRenderer.reset();
	this->mpUIRenderer.reset();
	this->mpRenderer.reset();
}

// Runs on the render thread
void Client::updateWorldGraphics()
{
	OPTICK_EVENT();
	if (this->mpVoxelInstanceBuffer && this->mpVoxelInstanceBuffer->hasChanges())
	{
		this->mpVoxelInstanceBuffer->lock();
		this->mpVoxelInstanceBuffer->commitToBuffer(&this->mpRenderer->getTransientPool());
		this->mpVoxelInstanceBuffer->unlock();
	}
	if (this->mpUIRenderer->hasChanges())
	{
		this->mpUIRenderer->commitWidgets();
	}
	if (this->mpEntityInstanceBuffer->hasChanges())
	{
		this->mpEntityInstanceBuffer->commitToBuffer(&this->mpRenderer->getTransientPool());
	}
}

void Client::bindInput()
{
	auto pInput = engine::Engine::Get()->getInputQueue();
	pInput->OnInputEvent.bind(
		input::EInputType::KEY, this->weak_from_this(),
		std::bind(&Client::onInputKey, this, std::placeholders::_1)
	);
}

void Client::unbindInput()
{
	auto pInput = engine::Engine::Get()->getInputQueue();
	pInput->OnInputEvent.unbind(input::EInputType::KEY, this->weak_from_this());
}

void Client::onInputKey(input::Event const& evt)
{
	if (evt.inputKey.action != input::EAction::RELEASE) return;
	if (input::isTextInputActive()) return;
	if (evt.inputKey.key == input::EKey::NUM_1)
	{
		//this->mProjectLog.log(LOG_INFO, "Regenerate");
		//this->mpWorld->reloadChunk({ 0, 0, 0 });
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

void Client::onWorldSimulate(f32 const& deltaTime)
{
	auto* network = game::Game::networkInterface();
	if (this->mLocalPlayerEntityId && network->isRunning() && network->type() == network::EType::eClient)
	{
		auto* transform = evcs::Core::Get()->entities().get(
			this->mLocalPlayerEntityId.value()
		)->getComponent<evcs::component::CoordinateTransform>();
		if ((this->mPrevLocalEntityPosition - transform->position()).toGlobal().magnitudeSq() > math::epsilon())
		{
			auto packet = network::packet::DisplacePlayer::create();
			packet->setTransformNetId(transform->netId()).moveTo(transform->position());
			packet->sendToServer();
			this->mPrevLocalEntityPosition = transform->position();
		}
	}
}
