#include "game/Game.hpp"

#include "Engine.hpp"
#include "Window.hpp"
#include "asset/BlockType.hpp"
#include "asset/Font.hpp"
#include "asset/Project.hpp"
#include "asset/RenderPassAsset.hpp"
#include "asset/Texture.hpp"
#include "asset/TextureSampler.hpp"
#include "ecs/Core.hpp"
#include "ecs/Entity.hpp"
#include "ecs/component/Transform.hpp"
#include "controller/Controller.hpp"
#include "graphics/GameRenderer.hpp"
#include "graphics/StringRenderer.hpp"
#include "graphics/Uniform.hpp"
#include "math/Vector.hpp"
#include "math/Matrix.hpp" 
#include "render/RenderCube.hpp"
#include "utility/StringUtils.hpp"

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
	this->destroyWindow();
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
		800, 800,
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
struct ModelViewProjection
{
	math::Matrix4x4 view;
	glm::mat4 proj;

	ModelViewProjection()
	{
		view = math::Matrix4x4(1);
		proj = glm::mat4(1);
	}
};

void Game::createRenderers()
{
	auto pEngine = engine::Engine::Get();
	this->mpRenderer = std::make_shared<graphics::GameRenderer>();
	pEngine->initializeRenderer(this->mpRenderer.get(), this->mpWindow);

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

	// TODO: Use dedicated graphics memory
	this->mpRendererMVP = graphics::Uniform::create<ModelViewProjection>(pEngine->getMiscMemory());
	this->mpRenderer->setStaticUniform(this->mpRendererMVP);
	
	// TODO: Load the render pass asset via a path stored on the project
	this->mpRenderer->setRenderPass(engine::Engine::Get()->getProject()->mRenderPass.load(asset::EAssetSerialization::Binary));

	// Setup UI Shader Pipeline
	{
		this->mpRenderer->setBindings(1,
			{
				graphics::AttributeBinding(graphics::AttributeBinding::Rate::eVertex)
				.setStructType<graphics::Font::UIVertex>()
				.addAttribute({ 0, /*vec4*/ (ui32)vk::Format::eR32G32B32A32Sfloat, offsetof(graphics::Font::UIVertex, position) })
				.addAttribute({ 1, /*vec2*/ (ui32)vk::Format::eR32G32Sfloat, offsetof(graphics::Font::UIVertex, texCoord) })
			}
		);
	}
	// Update font texture
	{
		auto fontAsset = asset::TypedAssetPath<asset::Font>::Create(
			"assets/font/Montserrat Regular.te-asset"
		).load(asset::EAssetSerialization::Binary);
		assert(fontAsset->supportsFontSize(48));
		auto stringRenderer = this->mpRenderer->setFont(fontAsset);
		auto renderedString = stringRenderer->makeGlobalString(48, { -1, -1 }, "Sphinx of black quartz, Judge my vow");
		this->mpCameraForwardStr = stringRenderer->makeGlobalString(48, { -1, -1 + 0.1f }, "<0, 0, 0>");
		this->mpRenderer->prepareUIBuffers(1000);
	}

	// Setup the object render instances
	{
		this->mpCubeRender = std::make_shared<RenderCube>();
		auto instances = std::vector<WorldObject>();
		{
			for (i32 x = -3; x <= 3; ++x)
				for (i32 y = -3; y <= 3; ++y)
					instances.push_back(
						WorldObject()
						.setPosition(glm::vec3(x, y, 0))
					);
		}
		// TODO: Expose pipeline object so user can set the shaders, attribute bindings, and uniforms directly
		// TODO: Vertex bindings should validate against the shader
		{
			auto bindings = std::vector<graphics::AttributeBinding>();
			ui8 slot = 0;
			this->mpCubeRender->appendBindings(bindings, slot);
			{
				auto additionalBindings = WorldObject::bindings(slot);
				bindings.insert(
					std::end(bindings),
					std::begin(additionalBindings),
					std::end(additionalBindings)
				);
			}
			this->mpRenderer->setBindings(0, bindings);
		}
		this->mpCubeRender->init(this->mpRenderer.get(), instances);
		this->mpRenderer->addRender(this->mpCubeRender.get());
	}

	// Add textures
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

	this->mpRenderer->createRenderChain();
	this->mpRenderer->finalizeInitialization();

	this->mpWindow->setRenderer(this->mpRenderer.get());
}

void Game::destroyRenderers()
{
	this->mpCameraForwardStr.reset();

	this->mpCubeRender->invalidate();
	this->mpCubeRender.reset();

	this->mpRenderer->invalidate();
	this->mpRenderer.reset();
}

void Game::createScene()
{
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
		if (i == 0)
		{
			auto rot = this->mpCameraTransform->orientation.euler() * math::rad2deg();
			auto fwd = this->mpCameraTransform->forward();
			this->mpCameraForwardStr.lock()->content(
				utility::formatStr("<%.2f, %.2f, %.2f>", fwd.x(), fwd.y(), fwd.z())
			);
		}

		{
			auto uniData = this->mpRendererMVP->read<ModelViewProjection>();
			uniData.view = this->mpCameraTransform->calculateView();
			uniData.proj = glm::perspective(glm::radians(45.0f), this->mpRenderer->getAspectRatio(), /*near plane*/ 0.1f, /*far plane*/ 100.0f);
			uniData.proj[1][1] *= -1; // sign flip b/c glm was made for OpenGL where the Y coord is inverted compared to Vulkan
			this->mpRendererMVP->write(&uniData);
		}

		pEngine->update(deltaTime);

		auto nextTime = std::chrono::high_resolution_clock::now();
		deltaTime = std::chrono::duration<float, std::chrono::seconds::period>(nextTime - prevTime).count();
		prevTime = nextTime;
		i = (i + 1) % 6000;
	}
	pEngine->joinThreads();
}
