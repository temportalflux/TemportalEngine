#include "Game.hpp"

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
#include "ecs/component/ComponentPlayerModel.hpp"
#include "ecs/view/ViewPlayerInputMovement.hpp"
#include "ecs/view/ViewPlayerCamera.hpp"
#include "ecs/view/ViewDebugHUD.hpp"
#include "ecs/view/ViewRenderedPlayer.hpp"
#include "ecs/system/SystemMovePlayerByInput.hpp"
#include "ecs/system/SystemUpdateCameraPerspective.hpp"
#include "ecs/system/SystemUpdateDebugHUD.hpp"
#include "ecs/system/SystemRenderPlayer.hpp"
#include "graphics/DescriptorPool.hpp"
#include "input/Queue.hpp"
#include "math/Vector.hpp"
#include "math/Matrix.hpp"
#include "registry/VoxelType.hpp"
#include "render/MinecraftRenderer.hpp"
#include "world/BlockInstanceMap.hpp"
#include "render/EntityInstanceBuffer.hpp"
#include "render/VoxelModelManager.hpp"
#include "render/VoxelGridRenderer.hpp"
#include "render/TextureRegistry.hpp"
#include "render/line/SimpleLineRenderer.hpp"
#include "render/line/ChunkBoundaryRenderer.hpp"
#include "render/model/SkinnedModelManager.hpp"
#include "render/ui/UIRenderer.hpp"
#include "utility/StringUtils.hpp"
#include "world/World.hpp"

#include <chrono>

using namespace game;

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
	auto memoryChunkSizes = utility::parseArgumentInts(args, "memory-", totalMem);
	engine::Engine::Create(memoryChunkSizes);
	this->initializeAssetTypes();
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
	ecs->components().registerType<ecs::component::PlayerModel>("PlayerModel");
	ecs->views().registerType<ecs::view::PlayerInputMovement>();
	ecs->views().registerType<ecs::view::PlayerCamera>();
	ecs->views().registerType<ecs::view::DebugHUD>();
	ecs->views().registerType<ecs::view::RenderedPlayer>();
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

void Game::initializeNetwork()
{
	// TODO: STUB
}

void Game::init()
{
	this->createVoxelTypeRegistry();
	if (this->requiresGraphics())
	{
		if (!this->initializeGraphics()) return;
		if (!this->createWindow()) return;
		this->createRenderers();
	}
	this->createScene();
	this->bindInput();
}

void Game::uninit()
{
	this->unbindInput();
	this->destroyScene();
	if (this->requiresGraphics())
	{
		this->destroyRenderers();
		this->destroyWindow();
	}
	this->destroyVoxelTypeRegistry();
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

void Game::destroyWindow()
{
	if (this->mpWindow)
	{
		engine::Engine::Get()->destroyWindow(this->mpWindow);
		this->mpWindow.reset();
	}
}

void Game::createRenderers()
{
	auto pEngine = engine::Engine::Get();

	this->createGameRenderer();
	this->loadVoxelTypeTextures();
	
	this->mpSystemUpdateCameraPerspective = pEngine->getMainMemory()->make_shared<ecs::system::UpdateCameraPerspective>(
		pEngine->getMiscMemory(), this->mpRenderer
	);
	pEngine->addTicker(this->mpSystemUpdateCameraPerspective);

	this->createPipelineRenderers();

	this->mpSystemUpdateDebugHUD = pEngine->getMainMemory()->make_shared<ecs::system::UpdateDebugHUD>(this->mpWindow);
	this->mpSystemUpdateDebugHUD->createHUD(this->mpUIRenderer);
	pEngine->addTicker(this->mpSystemUpdateDebugHUD);

	this->mpSkinnedModelManager = std::make_shared<graphics::SkinnedModelManager>(
		this->mpRenderer->getDevice(), &this->mpRenderer->getTransientPool()
	);
	this->mpEntityInstanceBuffer = std::make_shared<graphics::EntityInstanceBuffer>();
	this->mpEntityInstanceBuffer->setDevice(this->mpRenderer->getDevice());
	this->mpEntityInstanceBuffer->create();

	this->mpSystemRenderPlayer = std::make_shared<ecs::system::RenderPlayer>(
		std::weak_ptr(this->mpSkinnedModelManager), this->mpGlobalDescriptorPool
	);
	pEngine->addTicker(this->mpSystemRenderPlayer);
	this->mpSystemRenderPlayer->setPipeline(asset::TypedAssetPath<asset::Pipeline>::Create(
		"assets/render/entity/RenderEntityPipeline.te-asset"
	));
	this->mpRenderer->addRenderer(this->mpSystemRenderPlayer.get());

	this->mpTextureRegistry = std::make_shared<graphics::TextureRegistry>(
		this->mpRenderer->getDevice(), &this->mpRenderer->getTransientPool()
	);
	this->mpTextureRegistry->registerImage(asset::SKIN_DEFAULT_MASCULINE);
	this->mpTextureRegistry->registerSampler(asset::SAMPLER_NEAREST_NEIGHBOR);

	this->mpRenderer->createRenderChain();
	this->mpRenderer->finalizeInitialization();
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
	this->mpVoxelModelManager = std::make_shared<game::VoxelModelManager>();
	this->mpVoxelModelManager->setSampler(asset::TypedAssetPath<asset::TextureSampler>(asset::SAMPLER_NEAREST_NEIGHBOR));
	this->mpVoxelModelManager->loadRegistry(this->mpVoxelTypeRegistry);
	this->mpVoxelModelManager->createTextures(this->mpRenderer->getDevice(), &this->mpRenderer->getTransientPool());
	this->mpVoxelModelManager->createModels(this->mpRenderer->getDevice(), &this->mpRenderer->getTransientPool());
}

void Game::createPipelineRenderers()
{
	this->mpGlobalDescriptorPool = std::make_shared<graphics::DescriptorPool>();
	this->mpGlobalDescriptorPool->setDevice(this->mpRenderer->getDevice());
	this->mpGlobalDescriptorPool->setPoolSize(64, {
		{ vk::DescriptorType::eUniformBuffer, 64 },
		{ vk::DescriptorType::eCombinedImageSampler, 64 },
	});
	this->mpGlobalDescriptorPool->setAllocationMultiplier(this->mpRenderer->getSwapChainImageViewCount());
	this->mpGlobalDescriptorPool->create();

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
		totalBlockCount, this->mpVoxelTypeRegistry->getIds()
	);
	this->mpVoxelInstanceBuffer->setDevice(this->mpRenderer->getDevice());
	this->mpVoxelInstanceBuffer->createBuffer();

	this->mpVoxelGridRenderer = std::make_shared<graphics::VoxelGridRenderer>(
		std::weak_ptr(this->mpGlobalDescriptorPool),
		std::weak_ptr(this->mpVoxelInstanceBuffer)
	);
	this->mpVoxelGridRenderer->setPipeline(asset::TypedAssetPath<asset::Pipeline>::Create(
		"assets/render/world/VoxelPipeline.te-asset"
	).load(asset::EAssetSerialization::Binary));
	this->mpRenderer->addRenderer(this->mpVoxelGridRenderer.get());
	this->mpVoxelGridRenderer->createVoxelDescriptorMapping(this->mpVoxelTypeRegistry, this->mpVoxelModelManager);
}

void Game::createWorldAxesRenderer()
{
	this->mpWorldAxesRenderer = std::make_shared<graphics::SimpleLineRenderer>(
		std::weak_ptr(this->mpGlobalDescriptorPool)
	);
	this->mpWorldAxesRenderer->setPipeline(asset::TypedAssetPath<asset::Pipeline>::Create(
		"assets/render/debug/PerspectiveLinePipeline.te-asset"
	));
	this->mpRenderer->addRenderer(this->mpWorldAxesRenderer.get());
	// Y: 0->1 green up
	this->mpWorldAxesRenderer->addLineSegment({ { 0, 0, 0 }, { 0, .5f, 0 }, { 0, 1, 0, 1 } });
	// X: 0->1 red right
	this->mpWorldAxesRenderer->addLineSegment({ { 0, 0, 0 }, { .5f, 0, 0 }, { 1, 0, 0, 1 } });
	// Z: 0->1 blue forward
	this->mpWorldAxesRenderer->addLineSegment({ { 0, 0, 0 }, { 0, 0, .5f }, { 0, 0, 1, 1 } });
	this->mpWorldAxesRenderer->createGraphicsBuffers(&this->mpRenderer->getTransientPool());
}

void Game::createChunkBoundaryRenderer()
{
	this->mpChunkBoundaryRenderer = std::make_shared<graphics::ChunkBoundaryRenderer>(
		std::weak_ptr(this->mpGlobalDescriptorPool)
	);
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
		this->mpChunkBoundaryRenderer->setBoundarySegments(graphics::ChunkBoundaryType::eCube, segments, true);
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

		this->mpChunkBoundaryRenderer->setBoundarySegments(graphics::ChunkBoundaryType::eSideGrid, segments, true);
	}
	this->mpChunkBoundaryRenderer->createGraphicsBuffers(&this->mpRenderer->getTransientPool());
}

void Game::createUIRenderer()
{
	this->mpUIRenderer = std::make_shared<graphics::UIRenderer>(
		std::weak_ptr(this->mpGlobalDescriptorPool),
		/*maximum displayed characters=*/ 256
	);

	this->mpUIRenderer->setTextPipeline(asset::TypedAssetPath<asset::Pipeline>::Create(
		"assets/render/ui/UIPipeline.te-asset"
	).load(asset::EAssetSerialization::Binary));

	this->mpUIRenderer->addFont("montserrat", asset::TypedAssetPath<asset::Font>::Create(
		"assets/font/Montserrat.te-asset"
	).load(asset::EAssetSerialization::Binary));

	this->mpUIRenderer->addFont("sicret", asset::TypedAssetPath<asset::Font>::Create(
		"assets/font/Sicret.te-asset"
	).load(asset::EAssetSerialization::Binary));

	this->mpRenderer->addRenderer(this->mpUIRenderer.get());
}

void Game::destroyRenderers()
{
	this->mpSystemRenderPlayer->destroy();
	this->mpTextureRegistry.reset();
	this->mpEntityInstanceBuffer.reset();
	this->mpSkinnedModelManager.reset();
	this->mpVoxelModelManager.reset();
	this->mpVoxelGridRenderer->destroyRenderDevices();
	this->mpWorldAxesRenderer->destroy();
	this->mpChunkBoundaryRenderer->destroy();
	this->mpVoxelInstanceBuffer.reset();
	this->mpGlobalDescriptorPool->invalidate();
	
	this->mpSystemUpdateCameraPerspective.reset();
	this->mpSystemUpdateDebugHUD.reset();
	this->mpUIRenderer->destroyRenderDevices();

	this->mpRenderer->invalidate();

	this->mpGlobalDescriptorPool.reset();
	this->mpSystemRenderPlayer.reset();
	this->mpVoxelGridRenderer.reset();
	this->mpWorldAxesRenderer.reset();
	this->mpChunkBoundaryRenderer.reset();
	this->mpUIRenderer.reset();
	this->mpRenderer.reset();
}

void Game::createScene()
{
	srand((ui32)time(0));
	
	auto coordinates = std::vector<world::Coordinate>();
	FOR_CHUNK_SIZE(i32, y) FOR_CHUNK_SIZE(i32, z) FOR_CHUNK_SIZE(i32, x)
	{
		coordinates.push_back(world::Coordinate({ 0, 0, 0 }, { x, y, z }));
	}
	this->mpVoxelInstanceBuffer->allocateCoordinates(coordinates);

	this->changeVoxelDemoSmol();

	this->mpWorld = std::make_shared<world::World>();
	//this->mpWorld->OnBlockChanged.bind(this->mpCubeRender, this->mpCubeRender->onBlockChangedListener());
	this->mpWorld->loadChunk({ 0, 0, 0 });

	auto pEngine = engine::Engine::Get();
	this->createLocalPlayer();
	this->mpSystemMovePlayerByInput = pEngine->getMainMemory()->make_shared<ecs::system::MovePlayerByInput>();
	pEngine->addTicker(this->mpSystemMovePlayerByInput);
	
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

	// Add Transform
	{
		auto transform = components.create<ecs::component::CoordinateTransform>();
		transform->setPosition(world::Coordinate(math::Vector3Int::ZERO, { 1, 1, 3 }));
		transform->setOrientation(math::Vector3unitY, 0); // force the camera to face forward (+Z)
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
	
		this->mpEntityLocalPlayer->addView(views.create<ecs::view::DebugHUD>());

		// This view enables the `RenderPlayer` system to run
		this->mpEntityLocalPlayer->addView(views.create<ecs::view::RenderedPlayer>());

		// Required by `view::RenderedPlayer`
		auto playerModel = components.create<ecs::component::PlayerModel>();
		playerModel->createModel(this->mpSkinnedModelManager);
		playerModel->createInstance(this->mpEntityInstanceBuffer);
		this->mpEntityLocalPlayer->addComponent(playerModel);
	}

}

void Game::destroyScene()
{
	this->mpSystemMovePlayerByInput.reset();
	this->mpEntityLocalPlayer.reset();
	this->mpWorld.reset();
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
	this->mpWorld->handleDirtyCoordinates();
	engine::Engine::Get()->update(deltaTime);
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
		this->mpUIRenderer->lock();
		this->mpUIRenderer->commitToBuffer(&this->mpRenderer->getTransientPool());
		this->mpUIRenderer->unlock();
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
		this->changeVoxelDemoSmol();
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

void Game::changeVoxelDemoSmol()
{
	this->mpVoxelInstanceBuffer->lock();
	auto allVoxelIdsSet = this->mpVoxelTypeRegistry->getIds();

	auto allVoxelIdOptions = std::vector<std::optional<game::BlockId>>();
	allVoxelIdOptions.push_back(std::nullopt);
	std::transform(
		std::begin(allVoxelIdsSet), std::end(allVoxelIdsSet),
		std::back_inserter(allVoxelIdOptions),
		[](game::BlockId const& id) { return std::optional<game::BlockId>(id); }
	);

	auto idCount = std::unordered_map<game::BlockId, uSize>();
	for (auto const& id : allVoxelIdsSet)
	{
		idCount.insert(std::make_pair(id, 0));
	}

	for (i32 x = 0; x < CHUNK_SIDE_LENGTH; ++x)
	{
		for (i32 z = 0; z < CHUNK_SIDE_LENGTH; ++z)
		{
			auto const& id = allVoxelIdOptions[(uSize)(rand() % allVoxelIdOptions.size())];
			this->mpVoxelInstanceBuffer->changeVoxelId(
				world::Coordinate({ 0, 0, 0 }, { x, 0, z }), id
			);
			if (id) idCount.at(*id)++;
		}
	}

	for (auto const& entry : idCount)
	{
		this->mProjectLog.log(LOG_INFO, "- %s = %i", entry.first.to_string().c_str(), entry.second);
	}

	this->mpVoxelInstanceBuffer->unlock();
}
