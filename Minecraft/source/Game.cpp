#include "Game.hpp"

#include "Engine.hpp"
#include "Window.hpp"
#include "asset/BlockType.hpp"
#include "asset/Font.hpp"
#include "asset/Project.hpp"
#include "asset/RenderPassAsset.hpp"
#include "asset/PipelineAsset.hpp"
#include "asset/Texture.hpp"
#include "asset/TextureSampler.hpp"
#include "ecs/Core.hpp"
#include "ecs/Entity.hpp"
#include "ecs/component/Transform.hpp"
#include "controller/Controller.hpp"
#include "graphics/DescriptorPool.hpp"
#include "graphics/StringRenderer.hpp"
#include "graphics/Uniform.hpp"
#include "input/Queue.hpp"
#include "math/Vector.hpp"
#include "math/Matrix.hpp"
#include "registry/VoxelType.hpp"
#include "render/MinecraftRenderer.hpp"
#include "world/BlockInstanceMap.hpp"
#include "render/VoxelGridRenderer.hpp"
#include "render/VoxelModelManager.hpp"
#include "render/line/LineRenderer.hpp"
#include "render/line/ChunkBoundaryRenderer.hpp"
#include "utility/StringUtils.hpp"
#include "world/World.hpp"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE // perspective needs to use [0,1] range for Vulkan
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
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
	pEngine->initializeECS();
	return true;
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

// UniformBufferObject (UBO) for turning world coordinates to clip space when rendering
struct ChunkViewProj
{
	math::Matrix4x4 view;
	glm::mat4 proj;
	math::Vector3 posOfCurrentChunk;
	math::Vector3 sizeOfChunkInBlocks;

	ChunkViewProj()
	{
		view = math::Matrix4x4(1);
		proj = glm::mat4(1);
		posOfCurrentChunk = math::Vector3({ 0, 0, 0 });
		sizeOfChunkInBlocks = math::Vector3({ CHUNK_SIDE_LENGTH, CHUNK_SIDE_LENGTH, CHUNK_SIDE_LENGTH });
	}
};

struct LocalCamera
{
	math::Matrix4x4 view;
	glm::mat4 proj;

	LocalCamera()
	{
		view = math::Matrix4x4(1);
		proj = glm::mat4(1);
	}
};

void Game::createRenderers()
{
	this->createGameRenderer();
	this->loadVoxelTypeTextures();
	this->createPipelineRenderers();
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
	this->mpVoxelModelManager->setSampler(asset::TypedAssetPath<asset::TextureSampler>::Create("assets/textures/NearestNeighborSampler.te-asset"));
	this->mpVoxelModelManager->loadRegistry(this->mpVoxelTypeRegistry);
	this->mpVoxelModelManager->createTextures(this->mpRenderer->getDevice(), &this->mpRenderer->getTransientPool());
	this->mpVoxelModelManager->createModels(this->mpRenderer->getDevice(), &this->mpRenderer->getTransientPool());
}

void Game::createPipelineRenderers()
{
	// TODO: Use dedicated graphics memory
	this->mpRendererMVP = graphics::Uniform::create<ChunkViewProj>(engine::Engine::Get()->getMiscMemory());
	this->mpRenderer->addMutableUniform("mvpUniform", this->mpRendererMVP);
	this->mpUniformLocalCamera = graphics::Uniform::create<LocalCamera>(engine::Engine::Get()->getMiscMemory());
	this->mpRenderer->addMutableUniform("localCamera", this->mpUniformLocalCamera);

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
	
	// Setup UI Shader Pipeline
	//{
	//	this->mpRenderer->setBindings(1,
	//		{
	//			graphics::AttributeBinding(graphics::AttributeBinding::Rate::eVertex)
	//			.setStructType<graphics::Font::UIVertex>()
	//			.addAttribute({ 0, /*vec4*/ (ui32)vk::Format::eR32G32B32A32Sfloat, offsetof(graphics::Font::UIVertex, position) })
	//			.addAttribute({ 1, /*vec2*/ (ui32)vk::Format::eR32G32Sfloat, offsetof(graphics::Font::UIVertex, texCoord) })
	//		}
	//	);
	//}
	//// Update font texture
	//{
	//	auto fontAsset = asset::TypedAssetPath<asset::Font>::Create(
	//		"assets/font/Montserrat Regular.te-asset"
	//	).load(asset::EAssetSerialization::Binary);
	//	assert(fontAsset->supportsFontSize(48));
	//	auto stringRenderer = this->mpRenderer->setFont(fontAsset);
	//	auto renderedString = stringRenderer->makeGlobalString(48, { -1, -1 }, "Sphinx of black quartz, Judge my vow");
	//	this->mpCameraForwardStr = stringRenderer->makeGlobalString(48, { -1, -1 + 0.1f }, "<0, 0, 0>");
	//	this->mpRenderer->prepareUIBuffers(1000);
	//}

	// Add textures
	/*
	{
		auto sampler = asset::TypedAssetPath<asset::TextureSampler>::Create(
			"assets/textures/NearestNeighborSampler.te-asset"
		).load(asset::EAssetSerialization::Binary);
		auto idxSampler = this->mpRenderer->createTextureSampler(sampler);

		auto dirtTexture = asset::TypedAssetPath<asset::Texture>::Create(
			"assets/textures/block/Dirt.te-asset"
		).load(asset::EAssetSerialization::Binary);
		auto idxTex = this->mpRenderer->createTextureAssetImage(dirtTexture, idxSampler);
		this->mpRenderer->allocateTextureMemory(); // allocates the memory for all images created
		this->mpRenderer->writeTextureData(idxTex, dirtTexture);
	}
	//*/


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
	this->mpWorldAxesRenderer = std::make_shared<graphics::LineRenderer>(
		std::weak_ptr(this->mpGlobalDescriptorPool)
	);
	this->mpWorldAxesRenderer->setPipeline(asset::TypedAssetPath<asset::Pipeline>::Create(
		"assets/render/debug/PerspectiveLinePipeline.te-asset"
	).load(asset::EAssetSerialization::Binary));
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
	).load(asset::EAssetSerialization::Binary));
	this->mpRenderer->addRenderer(this->mpChunkBoundaryRenderer.get());

	f32 const l = CHUNK_SIDE_LENGTH;
	auto const axisSides = std::vector<f32>({ 0, l });

	// Columns
	{
		f32 const h = CHUNK_SIDE_LENGTH * 16;
		math::Vector4 color = { 0, 1, 0, 1 };
		auto segments = std::vector<graphics::LineSegment>();
		segments.push_back({ { 0, 0, 0 }, { 0, h, 0 }, color });
		segments.push_back({ { l, 0, 0 }, { l, h, 0 }, color });
		segments.push_back({ { l, 0, l }, { l, h, l }, color });
		segments.push_back({ { 0, 0, l }, { 0, h, l }, color });
		this->mpChunkBoundaryRenderer->setBoundarySegments(graphics::ChunkBoundaryType::eColumn, segments, true);
	}
	// Cube
	{
		math::Vector4 color = { 1, 0, 0, 1 };
		auto segments = std::vector<graphics::LineSegment>();
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
		auto segments = std::vector<graphics::LineSegment>();

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

void Game::destroyRenderers()
{
	//this->mpCameraForwardStr.reset();
	
	this->mpVoxelModelManager.reset();
	this->mpVoxelGridRenderer->destroyRenderDevices();
	this->mpWorldAxesRenderer->destroyBuffers();
	this->mpChunkBoundaryRenderer->destroyBuffers();
	this->mpVoxelInstanceBuffer.reset();
	this->mpGlobalDescriptorPool->invalidate();

	this->mpRenderer->invalidate();

	this->mpGlobalDescriptorPool.reset();
	this->mpVoxelGridRenderer.reset();
	this->mpWorldAxesRenderer.reset();
	this->mpChunkBoundaryRenderer.reset();
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
	/*
	auto camera = pEngine->getECS().createEntity();
	{
		//auto comp = pEngine->getECS().create<ecs::ComponentTransform>();
		//camera->components[0] = { ecs::ComponentTransform::TypeId, comp->id };
		//camera->componentCount++;
	}
	//*/

	this->mpCameraTransform = std::make_shared<ecs::ComponentTransform>();
	this->mpCameraTransform->setPosition(math::Vector3unitY * 3);
	this->mpCameraTransform->setOrientation(math::Vector3unitY, 0); // force the camera to face forward (+Z)
	auto fwd = this->mpCameraTransform->forward();
	this->mProjectLog.log(LOG_INFO, "<%.2f, %.2f, %.2f>", fwd.x(), fwd.y(), fwd.z());

	// TODO: Allocate from entity pool
	this->mpController = pEngine->getMainMemory()->make_shared<Controller>();
	pEngine->addTicker(this->mpController);
	this->mpController->subscribeToInput();
	this->mpController->assignCameraTransform(this->mpCameraTransform.get());

	if (this->mpVoxelInstanceBuffer)
	{
		while (this->mpVoxelInstanceBuffer->hasChanges())
		{
			// TODO: This will have to be adjusted when this function is asynchronous to wait for each commit to finish on the GPU
			this->mpVoxelInstanceBuffer->commitToBuffer(&this->mpRenderer->getTransientPool());
		}
	}
}

void Game::destroyScene()
{
	this->mpController.reset();
	this->mpCameraTransform.reset();
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
	ui32 i = 0;
	while (pEngine->isActive())
	{
		this->update(deltaTime);
		auto nextTime = std::chrono::high_resolution_clock::now();
		deltaTime = std::chrono::duration<float, std::chrono::seconds::period>(nextTime - prevTime).count();
		prevTime = nextTime;
		i = (i + 1) % 6000;
	}
	pEngine->joinThreads();
}

void Game::update(f32 deltaTime)
{
	OPTICK_EVENT();

	auto pEngine = engine::Engine::Get();

	/*
	if (i == 0)
	{
		auto rot = this->mpCameraTransform->orientation.euler() * math::rad2deg();
		auto fwd = this->mpCameraTransform->forward();
		this->mpCameraForwardStr.lock()->content(
			utility::formatStr("<%.2f, %.2f, %.2f>", fwd.x(), fwd.y(), fwd.z())
		);
	}
	//*/

	if (this->mpRenderer)
	{
		this->updateCameraUniform();
	}

	this->mpWorld->handleDirtyCoordinates();
	pEngine->update(deltaTime);

}

void Game::updateCameraUniform()
{
	OPTICK_EVENT();

	auto perspective = glm::perspective(glm::radians(45.0f), this->mpRenderer->getAspectRatio(), /*near plane*/ 0.01f, /*far plane*/ 100.0f);
	// GLM by default is not Y-up Right-Handed, so we have to flip the x and y coord bits
	// that said, this tweet says differently... https://twitter.com/FreyaHolmer/status/644881436982575104
	perspective[1][1] *= -1;
	perspective[0][0] *= -1;

	if (this->mpRendererMVP)
	{
		auto uniData = this->mpRendererMVP->read<ChunkViewProj>();
		uniData.view = this->mpCameraTransform->calculateView();
		uniData.proj = perspective;
		//uniData.chunkPos = { 0, 0, 0 };
		this->mpRendererMVP->write(&uniData);
	}

	if (this->mpUniformLocalCamera)
	{
		auto localCamera = this->mpUniformLocalCamera->read<LocalCamera>();
		localCamera.view = this->mpCameraTransform->calculateViewFrom(this->mpCameraTransform->position);
		localCamera.proj = perspective;
		this->mpUniformLocalCamera->write(&localCamera);
	}
}

// Runs on the render thread
void Game::updateWorldGraphics()
{
	if (this->mpVoxelInstanceBuffer->hasChanges())
	{
		this->mpVoxelInstanceBuffer->lock();
		this->mpVoxelInstanceBuffer->commitToBuffer(&this->mpRenderer->getTransientPool());
		this->mpVoxelInstanceBuffer->unlock();
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

	for (i32 x = CHUNK_HALF_LENGTH - 2; x <= CHUNK_HALF_LENGTH + 2; ++x)
	{
		for (i32 z = CHUNK_HALF_LENGTH - 2; z <= CHUNK_HALF_LENGTH + 2; ++z)
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
