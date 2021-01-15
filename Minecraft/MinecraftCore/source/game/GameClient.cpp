#include "game/GameClient.hpp"

#include "Engine.hpp"
#include "Window.hpp"
#include "asset/Font.hpp"
#include "asset/ModelAsset.hpp"
#include "asset/RenderPassAsset.hpp"
#include "asset/PipelineAsset.hpp"
#include "asset/Texture.hpp"
#include "asset/TextureSampler.hpp"
#include "asset/MinecraftAssetStatics.hpp"
#include "command/CommandRegistry.hpp"
#include "ecs/system/SystemUpdateCameraPerspective.hpp"
#include "ecs/system/SystemRenderEntities.hpp"
#include "game/GameInstance.hpp"
#include "game/GameWorldLogic.hpp"
#include "network/packet/NetworkPacketLoginWithAuthId.hpp"
#include "network/packet/NetworkPacketUpdateUserInfo.hpp"
#include "render/EntityInstanceBuffer.hpp"
#include "render/MinecraftRenderer.hpp"
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

using namespace game;

static auto CLIENT_LOG = DeclareLog("GameClient");

Client::Client() : Session(), mLocalUserNetId(std::nullopt)
{
	this->registerCommands();
	this->userRegistry().setLimit(1);
	this->userRegistry().scan("users");
	this->mLocalUserId = this->userRegistry().getUserCount() > 0
		? this->userRegistry().getId(0)
		: this->userRegistry().createUser();
}

void Client::init()
{
	if (!this->initializeGraphics()) return;
	if (!this->createWindow()) return;
	if (!this->scanResourcePacks()) return;
	this->createRenderers();
	this->mpResourcePackManager->loadPack("Default", 0).loadPack("Temportal", 1).commitChanges();
	this->mpSystemRenderEntities->createLocalPlayerDescriptor();
	this->mpRenderer->createMutableUniforms();
	this->mpRenderer->createRenderChain();
	this->mpRenderer->finalizeInitialization();
}

void Client::uninit()
{
	this->destroyRenderers();
	this->mpResourcePackManager.reset();
	this->destroyWindow();
}

void Client::registerCommands()
{
	auto registry = engine::Engine::Get()->commands();
	registry->add(
		command::Signature("setDPI")
		.pushArgType<ui32>() // dots per inch
		.bind([&](command::Signature const& cmd)
		{
			this->renderer()->setDPI(cmd.get<ui32>(0));
		})
	);
	registry->add(
		command::Signature("setName")
		.pushArgType<std::string>()
		.bind([&](command::Signature const& cmd)
		{
			auto name = cmd.get<std::string>(0);
			auto userInfo = this->localUserInfo();
			userInfo.setName(name);
			userInfo.writeToDisk();
			if (Game::networkInterface()->hasConnection())
			{
				network::packet::UpdateUserInfo::create()->setInfo(userInfo).sendToServer();
			}
		})
	);
	registry->add(
		command::Signature("id")
		.bind([&](command::Signature const& cmd)
		{
			auto userInfo = this->localUserInfo();
			std::stringstream ss;
			ss
				<< "Id: " << this->localUserId().toString().c_str()
				<< '\n'
				<< "Name: " << userInfo.name().c_str()
				;
			this->chat()->addToLog(ss.str());
		})
	);
	registry->add(
		command::Signature("listUsers")
		.bind([&](command::Signature const& cmd)
		{
			for (auto const&[netId, user] : this->connectedUsers())
			{
				// TODO this->chat()->addToLog(user.name);
			}
		})
	);
#pragma region Dedicated Servers
	registry->add(
		command::Signature("join")
		.pushArgType<network::Address>()
		.bind([&](command::Signature const& cmd)
		{
			this->setupNetwork(cmd.get<network::Address>(0));
			Game::networkInterface()->start();
		})
	);
	registry->add(
		command::Signature("joinLocal")
		.bind([&](command::Signature const& cmd)
		{
			auto localAddress = network::Address().setLocalTarget(ServerSettings().port());
			this->setupNetwork(localAddress);
			Game::networkInterface()->start();
		})
	);
	registry->add(
		command::Signature("leave")
		.bind(std::bind(&network::Interface::stop, Game::networkInterface()))
	);
#pragma endregion
#pragma region Integrated Servers / ClientOnTopOfServer
	registry->add(
		command::Signature("startHost")
		.bind([&](command::Signature const& cmd)
		{
			game::Game::Get()->setupNetworkServer({ network::EType::eServer, network::EType::eClient });
			Game::networkInterface()->start();
		})
	);
	registry->add(
		command::Signature("stopHost")
		.bind([&](command::Signature const& cmd)
		{
			game::Game::Get()->server().reset();
		})
	);
#pragma endregion

}

void Client::setupNetwork(network::Address const& serverAddress)
{
	auto& networkInterface = *Game::networkInterface();
	// Dedicated clients need to set the local user net id when its received from the server
	networkInterface.onNetIdReceived.bind(std::bind(
		&game::Client::onNetIdReceived, this,
		std::placeholders::_1, std::placeholders::_2
	));
	networkInterface.OnClientAuthenticated.bind(std::bind(
		&game::Client::onClientAuthenticated, this, std::placeholders::_1
	));
	networkInterface.onClientPeerStatusChanged.bind(std::bind(
		&game::Client::onClientPeerStatusChanged, this,
		std::placeholders::_1, std::placeholders::_2, std::placeholders::_3
	));
	networkInterface.OnClientDisconnected.bind(std::bind(
		&game::Client::OnClientDisconnected, this,
		std::placeholders::_1, std::placeholders::_2
	));
	networkInterface.setType(network::EType::eClient).setAddress(serverAddress);
}

void Client::onLocalServerConnectionOpened(network::Interface *pInterface, ui32 connection, ui32 netId)
{
	// Integrated ClientServer automatically initializes its local user
	this->setLocalUserNetId(netId);
}

void Client::onNetIdReceived(network::Interface *pInterface, ui32 netId)
{
	this->setLocalUserNetId(netId);
	this->addConnectedUser(netId);
	network::packet::LoginWithAuthId::create()->setId(this->localUserId()).sendToServer();
}

void Client::onClientAuthenticated(network::Interface *pInteface)
{
	network::logger().log(LOG_INFO, "Network handshake complete");
	network::packet::UpdateUserInfo::create()->setInfo(this->localUserInfo()).sendToServer();
}

void Client::OnClientDisconnected(network::Interface *pInteface, ui32 invalidNetId)
{
	network::logger().log(LOG_INFO, "Disconnected from network, killing network interface.");
	Game::networkInterface()->stop();
}

void Client::onClientPeerStatusChanged(network::Interface *pInterface, ui32 netId, network::EClientStatus status)
{
	if (status == network::EClientStatus::eConnected) this->addConnectedUser(netId);
	else this->removeConnectedUser(netId);
}

void Client::setLocalUserNetId(ui32 netId)
{
	this->mLocalUserNetId = netId;
}

utility::Guid const& Client::localUserId() const
{
	return this->mLocalUserId;
}

crypto::RSAKey Client::localUserAuthKey() const
{
	return this->userRegistry().loadKey(this->localUserId());
}

game::UserInfo Client::localUserInfo() const
{
	return this->userRegistry().loadInfo(this->localUserId());
}

void Client::addConnectedUser(ui32 netId)
{
	assert(!this->hasConnectedUser(netId));
	Session::addConnectedUser(netId);
	this->mConnectedUserInfo.insert(std::make_pair(netId, game::UserInfo()));
}

void Client::removeConnectedUser(ui32 netId)
{
	if (this->hasConnectedUser(netId))
	{
		Session::removeConnectedUser(netId);
		this->mConnectedUserInfo.erase(this->mConnectedUserInfo.find(netId));
	}
}

game::UserInfo& Client::getConnectedUserInfo(ui32 netId)
{
	return this->mConnectedUserInfo.find(netId)->second;
}

bool Client::initializeGraphics()
{
	return engine::Engine::Get()->setupVulkan();
}

bool Client::createWindow()
{
	auto pEngine = engine::Engine::Get();
	this->mpWindow = pEngine->createWindow(
		1280, 720,
		pEngine->getProject()->getDisplayName(),
		WindowFlags::RENDER_ON_THREAD
	);
	if (!this->mpWindow) return false;
	pEngine->initializeVulkan(this->mpWindow->querySDLVulkanExtensions());
	this->mpWindow->showCursor(false);
	this->mpWindow->consumeCursor(true);
	return true;
}

std::shared_ptr<Window> Client::getWindow() { return this->mpWindow; }

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
	this->mpResourcePackManager->scanPacksIn(std::filesystem::absolute("../../resource-packs"));

	if (!this->mpResourcePackManager->hasPack("Default"))
	{
		CLIENT_LOG.log(LOG_ERR, "Failed to find default resource pack");
		return false;
	}

	return true;
}

std::shared_ptr<ui::FontOwner> Client::uiFontOwner() { return this->mpUIRenderer; }

void Client::createRenderers()
{
	auto pEngine = engine::Engine::Get();

	this->createGameRenderer();
	//this->loadVoxelTypeTextures();

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

	this->mpSystemUpdateCameraPerspective = pEngine->getMainMemory()->make_shared<ecs::system::UpdateCameraPerspective>(
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

	this->mpSystemRenderEntities = std::make_shared<ecs::system::RenderEntities>();
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
	this->mpRenderer = std::make_shared<graphics::MinecraftRenderer>();
	this->mpRenderer->setDPI(96);
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
	//this->mpVoxelModelManager->loadRegistry(game::Game::Get()->worldLogic()->voxelTypeRegistry());
	this->mpResourcePackManager->OnResourcesLoadedEvent.bind(this->mpVoxelModelManager, this->mpVoxelModelManager->onTexturesLoadedEvent());
}

void Client::createPipelineRenderers()
{
	//this->createVoxelGridRenderer();
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

	auto voxelTypes = game::Game::Get()->worldLogic()->voxelTypeRegistry();
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
