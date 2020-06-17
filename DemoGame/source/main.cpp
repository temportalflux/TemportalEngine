#include "logging/Logger.hpp"
#include "Engine.hpp"
#include "WindowFlags.hpp"
#include "Window.hpp"
#include "graphics/GameRenderer.hpp"
#include "graphics/ShaderModule.hpp"
#include "graphics/Uniform.hpp"
#include "WorldObject.hpp"
#include "RenderCube.hpp"
#include "controller/Controller.hpp"

#include "Camera.hpp"
#include "ecs/Core.hpp"
#include "ecs/component/Transform.hpp"
#include "FixedSortedArray.hpp"

#include "memory/MemoryChunk.hpp"
#include "utility/StringUtils.hpp"
#include "asset/AssetManager.hpp"
#include "asset/Shader.hpp"
#include "asset/TypedAssetPath.hpp"

#include <iostream>
#include <stdarg.h>
#include <time.h>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <chrono>

using namespace std;

// UniformBufferObject (UBO) for turning world coordinates to clip space when rendering
struct ModelViewProjection
{
	glm::mat4 view;
	glm::mat4 proj;

	ModelViewProjection()
	{
		view = glm::mat4(1);
		proj = glm::mat4(1);
	}
};

void initializeNetwork(engine::Engine *pEngine)
{
	char selection;
	string junk;
	cout << "Select (c)lient, (s)erver, or (n)one: ";
	cin >> selection;
	getline(cin, junk);
	switch (selection)
	{
	case 'c':
	case 'C':
	{
		string ip;
		ui16 port;
		cout << "Enter server IP: ";
		getline(cin, ip);
		cout << "Enter port: ";
		cin >> port;
		getline(cin, junk);
		pEngine->createClient(ip.c_str(), port);
		break;
	}
	case 's':
	case 'S':
	{
		ui16 port, maxClients;
		cout << "Enter port: ";
		cin >> port;
		getline(cin, junk);
		cout << "Enter max clients: ";
		cin >> maxClients;
		getline(cin, junk);
		pEngine->createServer(port, maxClients);
		break;
	}
	case 'n':
	case 'N':
	default:
		break;
	}
}

int main(int argc, char *argv[])
{
	auto args = utility::parseArguments(argc, argv);

	uSize totalMem = 0;
	auto memoryChunkSizes = utility::parseArgumentInts(args, "memory-", totalMem);

	std::string logFileName = "TemportalEngine_" + logging::LogSystem::getCurrentTimeString() + ".log";
	engine::Engine::LOG_SYSTEM.open(logFileName.c_str());

	/*
	// TMP: START
	{
		auto core = ecs::Core().setLog(DeclareLog("ECS"));
		core.registerType<ecs::ComponentTransform>("Transform");
		core.constructComponentPools();

		ecs::Identifier idTransform;
		{
			auto* transform = core.create<ecs::ComponentTransform>();
			idTransform = transform->id;
			auto* transformCopy = core.lookup<ecs::ComponentTransform>(transform->id);
		}
		core.destroyComponent<ecs::ComponentTransform>(idTransform);

		engine::Engine::LOG_SYSTEM.close();
		return 0;
	}
	// TMP: END */

	{
		auto pEngine = engine::Engine::Create(memoryChunkSizes);
		LogEngine(logging::ECategory::LOGINFO, "Saving log to %s", logFileName.c_str());

		if (!pEngine->initializeDependencies())
		{
			pEngine.reset();
			engine::Engine::Destroy();
			return 1;
		}

		{
			auto project = asset::TypedAssetPath<asset::Project>(
				asset::AssetPath("project", std::filesystem::absolute("DemoGame.te-project"), true)
				).load(asset::EAssetSerialization::Binary, false);
			pEngine->setProject(project);
			pEngine->getAssetManager()->scanAssetDirectory(project->getAssetDirectory(), asset::EAssetSerialization::Binary);
		}

		//initializeNetwork(pEngine);

		std::string title = "Demo Game";
		if (pEngine->hasNetwork())
		{
			/*
			auto network = pEngine->getNetworkService();
			if (network.has_value())
			{
				if (network.value()->isServer())
				{
					title += " (Server)";
				}
				else
				{
					title += " (Client)";
				}
			}
			//*/
		}

		if (!pEngine->setupVulkan())
		{
			pEngine.reset();
			engine::Engine::Destroy();
			return 1;
		}

		auto pWindow = pEngine->createWindow(
			800, 600,
			pEngine->getProject()->getDisplayName(),
			WindowFlags::RENDER_ON_THREAD | WindowFlags::RESIZABLE
		);
		if (pWindow == nullptr)
		{
			pEngine.reset();
			engine::Engine::Destroy();
			return 1;
		}
		pWindow->showCursor(false);
		pWindow->consumeCursor(true);

		{
			auto renderCube = RenderCube();
			auto instances = std::vector<WorldObject>();
			{
				for (i32 x = -3; x <= 3; ++x)
					for (i32 y = -3; y <= 3; ++y)
						instances.push_back(
							WorldObject()
							.setPosition(glm::vec3(x, y, 0))
						);
			}

			/*
			auto triangle = WorldObject();
			{
				plane.pushVertex({ {+0.0f, -0.5f}, {1.0f, 0.0f, 0.0f} });
				plane.pushVertex({ {+0.5f, +0.5f}, {0.0f, 1.0f, 0.0f} });
				plane.pushVertex({ {-0.5f, +0.5f}, {0.0f, 0.0f, 1.0f} });
				plane.pushIndex(0);
				plane.pushIndex(1);
				plane.pushIndex(2);
			}
			//*/

			// Will release memory allocated when it goes out of scope
			// TODO: Use dedicated graphics memory
			auto mvpUniform = graphics::Uniform::create<ModelViewProjection>(pEngine->getMiscMemory());
			
#pragma region Vulkan
			auto renderer = graphics::GameRenderer();
			pEngine->initializeVulkan(pWindow->querySDLVulkanExtensions());
			pEngine->initializeRenderer(&renderer, pWindow);

			// TODO: Configure this per project
			renderer.setSwapChainInfo(
				graphics::SwapChainInfo()
				.addFormatPreference(vk::Format::eB8G8R8A8Srgb)
				.setColorSpace(vk::ColorSpaceKHR::eSrgbNonlinear)
				.addPresentModePreference(vk::PresentModeKHR::eMailbox)
				.addPresentModePreference(vk::PresentModeKHR::eFifo)
			);

			// TODO: Configure this per image view (render target)
			// TODO: Image views only need the format from the swapchain. They can be created independently of it too.
			renderer.setImageViewInfo(
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

			renderer.setStaticUniform(mvpUniform);

			// Create shaders
			{
				auto assetManager = asset::AssetManager::get();
				// Vertex Shader from asset
				renderer.addShader(pEngine->getProject()->mVertexShader.load(asset::EAssetSerialization::Binary)->makeModule());
				// Fragment Shader from asset
				renderer.addShader(pEngine->getProject()->mFragmentShader.load(asset::EAssetSerialization::Binary)->makeModule());
			}

			// TODO: Expose pipeline object so user can set the shaders, attribute bindings, and uniforms directly
			// TODO: Vertex bindings should validate against the shader
			{
				auto bindings = std::vector<graphics::AttributeBinding>();
				ui8 slot = 0;
				renderCube.appendBindings(bindings, slot);
				{
					auto additionalBindings = WorldObject::bindings(slot);
					bindings.insert(
						std::end(bindings),
						std::begin(additionalBindings),
						std::end(additionalBindings)
					);
				}
				renderer.setBindings(bindings);
			}

			renderer.initializeBufferHelpers();
			renderCube.init(&renderer, instances);
			renderer.addRender(&renderCube);

			renderer.createRenderChain();
			renderer.finalizeInitialization();

			// Give the window its renderer
			pWindow->setRenderer(&renderer);
#pragma endregion

			// TODO: Allocate from entity pool
			auto camera = pEngine->getMainMemory()->make_shared<Camera>();
			camera->move(glm::vec3(0, 0, 10));

			// TODO: Allocate from entity pool
			auto controller = pEngine->getMainMemory()->make_shared<Controller>();
			pEngine->addTicker(controller);
			controller->subscribeToInput();
			controller->assignCamera(camera);

			pEngine->start();
			auto prevTime = std::chrono::high_resolution_clock::now();
			f32 deltaTime = 0.0f;
			while (pEngine->isActive())
			{
				//plane.rotate(glm::vec3(0, 0, 1), deltaTime * glm::radians(90.0f));
				{
					auto uniData = mvpUniform->read<ModelViewProjection>();
					auto fwd = camera->forward();
					uniData.view = glm::lookAt(camera->position(), camera->position() + fwd, camera->up());
					uniData.proj = glm::perspective(glm::radians(45.0f), renderer.getAspectRatio(), /*near plane*/ 0.1f, /*far plane*/ 100.0f);
					uniData.proj[1][1] *= -1; // sign flip b/c glm was made for OpenGL where the Y coord is inverted compared to Vulkan
					mvpUniform->write(&uniData);
				}

				pEngine->update(deltaTime);

				auto nextTime = std::chrono::high_resolution_clock::now();
				deltaTime = std::chrono::duration<float, std::chrono::seconds::period>(nextTime - prevTime).count();
				prevTime = nextTime;
			}
			pEngine->joinThreads();

			controller.reset();
			camera.reset();

			// TODO: Headless https://github.com/temportalflux/ChampNet/blob/feature/final/ChampNet/ChampNetPluginTest/source/StateApplication.cpp#L61

			renderCube.invalidate();
			renderer.invalidate();
		}

		engine::Engine::Get()->destroyWindow(pWindow);
		pWindow.reset();

		pEngine.reset();
		engine::Engine::Destroy();
	}

	engine::Engine::LOG_SYSTEM.close();
	return 0;
}