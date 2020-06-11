#include "logging/Logger.hpp"
#include "Engine.hpp"
#include "WindowFlags.hpp"
#include "Window.hpp"
#include "graphics/VulkanRenderer.hpp"
#include "graphics/ShaderModule.hpp"
#include "graphics/Uniform.hpp"

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

struct Vertex
{
	glm::vec2 position;
	glm::vec3 color;
};

// UniformBufferObject (UBO) for turning world coordinates to clip space when rendering
struct ModelViewProjection
{
	glm::mat4 model;
	glm::mat4 view;
	glm::mat4 proj;

	ModelViewProjection()
	{
		model = glm::mat4(1);
		view = glm::mat4(1);
		proj = glm::mat4(1);
	}
};

/*
const std::array<Vertex, 3> vertices = { {
	{{0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}},
	{{0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
	{{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}}
} };
//*/
const std::array<Vertex, 4> vertices = { {
	{{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
	{{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
	{{0.5f, 0.5f}, {1.0f, 0.0f, 0.0f}},
	{{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}}
} };
const std::array<ui16, 6> indices = { {
	0, 1, 2, 2, 3, 0
} };

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

	ui64 totalMem = 0;
	auto memoryChunkSizes = utility::parseArgumentInts(args, "memory-", totalMem);
	
	std::string logFileName = "TemportalEngine_" + logging::LogSystem::getCurrentTimeString() + ".log";
	engine::Engine::LOG_SYSTEM.open(logFileName.c_str());

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

		{
			// Will release memory allocated when it goes out of scope
			// TODO: Use dedicated graphics memory
			auto mvpUniform = graphics::Uniform::create<ModelViewProjection>(pEngine->getMiscMemory());

#pragma region Vulkan
			auto pVulkan = pEngine->initializeVulkan(pWindow->querySDLVulkanExtensions());
			// TODO: Wrap these methods in a renderer creation method in engine
			auto renderer = graphics::VulkanRenderer(pVulkan, pWindow->createSurface().initialize(pVulkan));
			renderer.setPhysicalDevicePreference(pEngine->getProject()->getPhysicalDevicePreference());
			renderer.setLogicalDeviceInfo(pEngine->getProject()->getGraphicsDeviceInitInfo());
#ifndef NDEBUG
			renderer.setValidationLayers(engine::Engine::VulkanValidationLayers);
#endif

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

			renderer.addUniform(mvpUniform);

			// Initialize required api connections
			renderer.initializeDevices();

			// Create shaders
			{
				auto assetManager = asset::AssetManager::get();
				// Vertex Shader from asset
				{
					std::shared_ptr<graphics::ShaderModule> shaderModule;
					// Make module from shader binary in asset
					{
						shaderModule = pEngine->getProject()->mVertexShader.load(asset::EAssetSerialization::Binary)->makeModule();
					}
					// Set the description for the input
					shaderModule->setVertexDescription(
						{
							sizeof(Vertex),
							{
								{ "position", CREATE_ATTRIBUTE(Vertex, position) },
								{ "color", CREATE_ATTRIBUTE(Vertex, color) }
							}
						}
					);
					renderer.addShader(shaderModule);
				}
				// Fragment Shader from asset
				renderer.addShader(pEngine->getProject()->mFragmentShader.load(asset::EAssetSerialization::Binary)->makeModule());
			}

			// Initialize the rendering api connections
			renderer.createInputBuffers(
				sizeof(Vertex) * (ui32)vertices.size(),
				sizeof(ui16) * (ui32)indices.size()
			);
			renderer.writeVertexData(0, vertices);
			renderer.writeIndexData(0, indices);

			renderer.createRenderChain();
			renderer.finalizeInitialization();

			// Give the window its renderer
			pWindow->setRenderer(&renderer);
#pragma endregion

			pEngine->start();
			//mvp.model = glm::mat4(1);
			auto prevTime = std::chrono::high_resolution_clock::now();
			float deltaTime = 0.0f;
			while (pEngine->isActive())
			{
				{
					auto uniData = mvpUniform->read<ModelViewProjection>();
					uniData.model = glm::rotate(uniData.model, deltaTime * glm::radians(90.0f), glm::vec3(0, 0, 1));
					uniData.view = glm::lookAt(glm::vec3(2), glm::vec3(0), glm::vec3(0, 0, 1));
					uniData.proj = glm::perspective(glm::radians(45.0f), renderer.getAspectRatio(), 0.1f, 10.0f);
					uniData.proj[1][1] *= -1; // sign flip b/c glm was made for OpenGL where the Y coord is inverted compared to Vulkan
					mvpUniform->write(&uniData);
				}

				pEngine->update();

				auto nextTime = std::chrono::high_resolution_clock::now();
				deltaTime = std::chrono::duration<float, std::chrono::seconds::period>(nextTime - prevTime).count();
				prevTime = nextTime;
			}
			pEngine->joinThreads();

			// TODO: Headless https://github.com/temportalflux/ChampNet/blob/feature/final/ChampNet/ChampNetPluginTest/source/StateApplication.cpp#L61

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