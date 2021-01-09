#include "game/GameInstance.hpp"

#include "Engine.hpp"
#include "Window.hpp"
#include "asset/BlockType.hpp"
#include "asset/Font.hpp"
#include "asset/ModelAsset.hpp"
#include "asset/Project.hpp"
#include "asset/RenderPassAsset.hpp"
#include "asset/PipelineAsset.hpp"
#include "asset/Texture.hpp"
#include "asset/TextureSampler.hpp"
#include "asset/MinecraftAssetStatics.hpp"
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
#include "ecs/system/SystemUpdateCameraPerspective.hpp"
#include "ecs/system/SystemRenderEntities.hpp"
#include "ecs/system/SystemPhysicsIntegration.hpp"
#include "graphics/DescriptorPool.hpp"
#include "input/Queue.hpp"
#include "math/Vector.hpp"
#include "math/Matrix.hpp"
#include "network/NetworkCore.hpp"
#include "physics/ChunkCollisionManager.hpp"
#include "physics/PhysicsMaterial.hpp"
#include "physics/PhysicsRigidBody.hpp"
#include "physics/PhysicsScene.hpp"
#include "physics/PhysicsShape.hpp"
#include "physics/PhysicsSystem.hpp"
#include "registry/VoxelType.hpp"
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
#include "utility/StringUtils.hpp"
#include "world/World.hpp"
#include "ui/DebugHUD.hpp"
#include "ui/TextLogMenu.hpp"
#include "ui/UIWidgets.hpp"

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

Game::Game(int argc, char *argv[])
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

	auto networkAddress = network::Address();

	auto clientParam = args.find("client");
	auto serverParam = args.find("server");
	if (clientParam != args.end() && clientParam->second)
	{
		if (networkAddress.fromString(clientParam->second.value()))
		{
			this->mNetworkInterface
				.setType(network::Interface::EType::eClient)
				.setAddress(networkAddress);
		}
	}
	else if (serverParam != args.end() && serverParam->second)
	{
		networkAddress.port() = (ui32)std::stoi(serverParam->second.value());
		this->mNetworkInterface
			.setType(network::Interface::EType::eServer)
			.setAddress(networkAddress);
	}
}

Game::~Game()
{
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
	this->mNetworkInterface.start();

	this->createVoxelTypeRegistry();

	this->mpPhysics = std::make_shared<physics::System>();
	this->mpPhysics->init(true);

	if (this->requiresGraphics())
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

	auto pEngine = engine::Engine::Get();

	this->mpSystemPhysicsIntegration = std::make_shared<ecs::system::PhysicsIntegration>();
	pEngine->addTicker(this->mpSystemPhysicsIntegration);
	
	this->createWorld();
	this->bindInput();
}

void Game::uninit()
{
	this->unbindInput();
	this->destroyWorld();
	this->mpSystemPhysicsIntegration.reset();
	if (this->requiresGraphics())
	{
		this->destroyRenderers();
		this->destroyWindow();
	}
	this->mpPhysics.reset();
	this->destroyVoxelTypeRegistry();
	this->mNetworkInterface.stop();
	network::uninit();
}

bool Game::scanResourcePacks()
{
	this->mpResourcePackManager = std::make_shared<resource::PackManager>();
	this->mpResourcePackManager->scanPacksIn(std::filesystem::absolute("../../resource-packs"));

	if (!this->mpResourcePackManager->hasPack("Default"))
	{
		this->mProjectLog.log(LOG_ERR, "Failed to find default resource pack");
		return false;
	}

	return true;
}

void Game::destroyResourcePacks()
{
	this->mpResourcePackManager.reset();
}

void Game::createVoxelTypeRegistry()
{
	static auto Log = DeclareLog("VoxelTypeRegistry");

	this->mpVoxelTypeRegistry = std::make_shared<game::VoxelTypeRegistry>();

	Log.log(LOG_INFO, "Gathering block types...");
	auto blockList = this->assetManager()->getAssetList<asset::BlockType>();
	Log.log(LOG_INFO, "Found %i block types", blockList.size());
	this->mpVoxelTypeRegistry->registerEntries(blockList);
}

void Game::destroyVoxelTypeRegistry()
{
	this->mpVoxelTypeRegistry.reset();
}

bool Game::requiresGraphics() const
{
	// TODO: Headless https://github.com/temportalflux/ChampNet/blob/feature/final/ChampNet/ChampNetPluginTest/source/StateApplication.cpp#L61
	return true; // TODO: Return false if not running a client (doesn't need to render game servers)
}

bool Game::initializeGraphics()
{
	return engine::Engine::Get()->setupVulkan();
}

bool Game::createWindow()
{
	auto pEngine = engine::Engine::Get();
	this->mpWindow = pEngine->createWindow(
		1280, 720,
		pEngine->getProject()->getDisplayName(),
		WindowFlags::RENDER_ON_THREAD | WindowFlags::RESIZABLE
	);
	if (!this->mpWindow) return false;
	pEngine->initializeVulkan(this->mpWindow->querySDLVulkanExtensions());
	this->mpWindow->showCursor(false);
	this->mpWindow->consumeCursor(true);
	return true;
}

std::shared_ptr<Window> Game::getWindow() { return this->mpWindow; }

void Game::destroyWindow()
{
	if (this->mpWindow)
	{
		engine::Engine::Get()->destroyWindow(this->mpWindow);
		this->mpWindow.reset();
	}
}

std::shared_ptr<ui::FontOwner> Game::uiFontOwner() { return this->mpUIRenderer; }

void Game::createRenderers()
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
		auto size = math::Vector2UInt { 16, 16 };
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
	this->mpMenuTextLog->addImagesToRenderer(this->mpUIRenderer.get());
}

void Game::createGameRenderer()
{
	this->mpRenderer = std::make_shared<graphics::MinecraftRenderer>();
	engine::Engine::Get()->initializeRenderer(this->mpRenderer.get(), this->mpWindow);
	this->mpWindow->setRenderer(this->mpRenderer.get());
	this->mpRenderer->UpdateWorldGraphicsOnFramePresented.bind(
		std::bind(&Game::updateWorldGraphics, this)
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

void Game::loadVoxelTypeTextures()
{
	this->mpVoxelModelManager = std::make_shared<game::VoxelModelManager>(
		this->mpRenderer->getDevice(), &this->mpRenderer->getTransientPool()
	);
	this->mpVoxelModelManager->setSampler(asset::TypedAssetPath<asset::TextureSampler>(asset::SAMPLER_NEAREST_NEIGHBOR));
	this->mpVoxelModelManager->loadRegistry(this->mpVoxelTypeRegistry);
	this->mpResourcePackManager->OnResourcesLoadedEvent.bind(this->mpVoxelModelManager, this->mpVoxelModelManager->onTexturesLoadedEvent());
}

void Game::createPipelineRenderers()
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

void Game::createVoxelGridRenderer()
{
	auto const totalBlockCount = blockCountForRenderDistance(6);
	this->mpVoxelInstanceBuffer = std::make_shared<world::BlockInstanceBuffer>(
		totalBlockCount, this->mpVoxelTypeRegistry
	);
	this->mpVoxelInstanceBuffer->setDevice(this->mpRenderer->getDevice());
	this->mpVoxelInstanceBuffer->createBuffer();

	this->mpVoxelGridRenderer = std::make_shared<graphics::VoxelGridRenderer>(
		&this->mpRenderer->getDescriptorPool(),
		std::weak_ptr(this->mpVoxelInstanceBuffer),
		this->mpVoxelTypeRegistry, this->mpVoxelModelManager
	);
	this->mpVoxelGridRenderer->setPipeline(asset::TypedAssetPath<asset::Pipeline>::Create(
		"assets/render/world/VoxelPipeline.te-asset"
	));
	this->mpRenderer->addRenderer(this->mpVoxelGridRenderer.get());
	this->mpVoxelModelManager->OnAtlasesCreatedEvent.bind(this->mpVoxelGridRenderer, this->mpVoxelGridRenderer->onAtlasesCreatedEvent());
}

void Game::createWorldAxesRenderer()
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

void Game::createChunkBoundaryRenderer()
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

void Game::createUIRenderer()
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

void Game::destroyRenderers()
{
	this->mpSystemRenderEntities->destroy();
	this->mpTextureRegistry.reset();
	this->mpEntityInstanceBuffer.reset();
	this->mpSkinnedModelManager.reset();
	this->mpVoxelModelManager.reset();
	this->mpVoxelGridRenderer->destroy();
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

void Game::createWorld()
{
	this->mpSceneOverworld = std::make_shared<physics::Scene>();
	this->mpSceneOverworld->setSystem(this->mpPhysics);
	this->mpSceneOverworld->create();

	this->mpChunkCollisionManager = std::make_shared<physics::ChunkCollisionManager>(
		this->mpPhysics, this->mpSceneOverworld
	);

	this->mpWorld = std::make_shared<world::World>(ui32(time(0)));
	if (this->mpVoxelInstanceBuffer) this->mpWorld->addEventListener(this->mpVoxelInstanceBuffer);
	this->mpWorld->addEventListener(this->mpChunkCollisionManager);
	
	this->mpPlayerPhysicsMaterial = std::make_shared<physics::Material>();
	this->mpPlayerPhysicsMaterial->setSystem(this->mpPhysics);
	this->mpPlayerPhysicsMaterial->create();

	this->createScene();
	this->createLocalPlayer();

	// Specifically for clients which set player movement/camera information
	{
		auto pEngine = engine::Engine::Get();
		this->mpSystemMovePlayerByInput = pEngine->getMainMemory()->make_shared<ecs::system::MovePlayerByInput>();
		pEngine->addTicker(this->mpSystemMovePlayerByInput);
	}

}

void Game::destroyWorld()
{
	this->destroyScene();
	this->mpPlayerPhysicsMaterial.reset();
	this->mpChunkCollisionManager.reset();
	this->mpSceneOverworld.reset();
	this->mpWorld.reset();
}

void Game::createScene()
{
	this->mpWorld->loadChunk({ 0, 0, 0 });
	this->mpWorld->loadChunk({ 0, 0, -1 });
	//for (i32 x = -1; x <= 1; ++x) for (i32 z = -1; z <= 1; ++z)
	//	this->mpWorld->loadChunk({ x, 0, z });
	
	if (this->mpVoxelInstanceBuffer)
	{
		while (this->mpVoxelInstanceBuffer->hasChanges())
		{
			// TODO: This will have to be adjusted when this function is asynchronous to wait for each commit to finish on the GPU
			this->mpVoxelInstanceBuffer->commitToBuffer(&this->mpRenderer->getTransientPool());
		}
	}
}

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

	if (this->requiresGraphics())
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

void Game::destroyScene()
{
	this->mpSystemMovePlayerByInput.reset();
	this->mpEntityLocalPlayer.reset();
}

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
	
	// TODO: move network comms to dedicated thread?
	this->mNetworkInterface.update(deltaTime);

	this->mpWorld->handleDirtyCoordinates();
	engine::Engine::Get()->update(deltaTime);
	this->mpSceneOverworld->simulate(deltaTime);
}

// Runs on the render thread
void Game::updateWorldGraphics()
{
	OPTICK_EVENT();
	if (this->mpVoxelInstanceBuffer->hasChanges())
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

void Game::onInputKey(input::Event const& evt)
{
	if (evt.inputKey.action != input::EAction::RELEASE) return;
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
