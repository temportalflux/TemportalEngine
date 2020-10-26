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
#include "graphics/StringRenderer.hpp"
#include "graphics/Uniform.hpp"
#include "math/Vector.hpp"
#include "math/Matrix.hpp"
#include "registry/VoxelType.hpp"
#include "render/MinecraftRenderer.hpp"
#include "render/RenderBlocks.hpp"
#include "render/VoxelGridRenderer.hpp"
#include "render/VoxelModelManager.hpp"
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
	auto project = asset::TypedAssetPath<asset::Project>(
		asset::AssetPath("project", std::filesystem::absolute("Minecraft.te-project"), true)
	).load(asset::EAssetSerialization::Binary, false);
	pEngine->setProject(project);
	pEngine->getAssetManager()->scanAssetDirectory(project->getAssetDirectory(), asset::EAssetSerialization::Binary);
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
}

void Game::uninit()
{
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
		sizeOfChunkInBlocks = math::Vector3({ 16, 16, 16 });
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

	auto renderPassAssetPath = engine::Engine::Get()->getProject()->mRenderPass;
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

	this->createVoxelGridRenderer();
	
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

void Game::createVoxelGridRenderer()
{
	this->mpVoxelGridRenderer = std::make_shared<graphics::VoxelGridRenderer>();
	this->mpVoxelGridRenderer->setPipeline(asset::TypedAssetPath<asset::Pipeline>::Create(
		"assets/WorldPipeline.te-asset"
	).load(asset::EAssetSerialization::Binary));
	this->mpRenderer->addRenderer(this->mpVoxelGridRenderer.get());
	this->mpVoxelGridRenderer->createVoxelDescriptorMapping(this->mpVoxelTypeRegistry, this->mpVoxelModelManager);
	this->mpVoxelGridRenderer->writeInstanceBuffer(&this->mpRenderer->getTransientPool());
}

void Game::destroyRenderers()
{
	//this->mpCameraForwardStr.reset();
	this->mpVoxelModelManager.reset();
	this->mpVoxelGridRenderer->destroyRenderDevices();
	this->mpRenderer->invalidate();
	this->mpVoxelGridRenderer.reset();
	this->mpRenderer.reset();
}

void Game::createScene()
{
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
	this->mpCameraTransform->setPosition(math::Vector3unitZ * 3);

	// TODO: Allocate from entity pool
	this->mpController = pEngine->getMainMemory()->make_shared<Controller>();
	pEngine->addTicker(this->mpController);
	this->mpController->subscribeToInput();
	this->mpController->assignCameraTransform(this->mpCameraTransform.get());
}

void Game::destroyScene()
{
	this->mpController.reset();
	this->mpCameraTransform.reset();
	this->mpWorld.reset();
}

void Game::run()
{
	auto pEngine = engine::Engine::Get();

	pEngine->start();
	auto prevTime = std::chrono::high_resolution_clock::now();
	f32 deltaTime = 0.0f;
	ui32 i = 0;
	while (pEngine->isActive())
	{
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

		{
			auto uniData = this->mpRendererMVP->read<ChunkViewProj>();
			uniData.view = this->mpCameraTransform->calculateView();
			uniData.proj = glm::perspective(glm::radians(45.0f), this->mpRenderer->getAspectRatio(), /*near plane*/ 0.1f, /*far plane*/ 100.0f);
			uniData.proj[1][1] *= -1; // sign flip b/c glm was made for OpenGL where the Y coord is inverted compared to Vulkan
			//uniData.chunkPos = { 0, 0, 0 };
			this->mpRendererMVP->write(&uniData);
		}

		this->mpWorld->handleDirtyCoordinates();
		pEngine->update(deltaTime);

		auto nextTime = std::chrono::high_resolution_clock::now();
		deltaTime = std::chrono::duration<float, std::chrono::seconds::period>(nextTime - prevTime).count();
		prevTime = nextTime;
		i = (i + 1) % 6000;
	}
	pEngine->joinThreads();
}
