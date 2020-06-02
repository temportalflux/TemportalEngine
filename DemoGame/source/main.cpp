#include "logging/Logger.hpp"
#include "Engine.hpp"
#include "WindowFlags.hpp"
#include "Window.hpp"
#include "graphics/VulkanRenderer.hpp"
#include "graphics/ShaderModule.hpp"

#include "memory/MemoryChunk.hpp"
#include "utility/StringUtils.hpp"

#include <array>
#include <filesystem>
#include <iostream>
#include <string>
#include <stdarg.h>
#include <time.h>
#include <unordered_map>
#include <glm/glm.hpp>

using namespace std;

struct Vertex
{
	glm::vec2 position;
	glm::vec3 color;
};

const std::array<Vertex, 3> vertices = { {
	{{0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}},
	{{0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
	{{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}}
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
	assert(totalMem > 0);
	
	auto memoryChunk = memory::MemoryChunk::Create(totalMem);

	std::string logFileName = "TemportalEngine_" + logging::LogSystem::getCurrentTimeString() + ".log";
	engine::Engine::LOG_SYSTEM.open(logFileName.c_str());

	{
		auto pEngine = engine::Engine::Create(memoryChunk, memoryChunkSizes);
		LogEngine(logging::ECategory::LOGINFO, "Saving log to %s", logFileName.c_str());

		if (!pEngine->initializeDependencies())
		{
			engine::Engine::Destroy();
			return 1;
		}

		// TODO: Create copies of all assets (project file and assets directory) in binary format and store them in the output folder
		{
			auto assetManager = pEngine->getAssetManager();
			auto asset = assetManager->readAssetFromDisk(std::filesystem::absolute("../../DemoGame/DemoGame.te-project"), false);
			asset::ProjectPtrStrong project = std::dynamic_pointer_cast<asset::Project>(asset);
			assert(project != nullptr);
			pEngine->setProject(project);
			assetManager->scanAssetDirectory(project->getAssetDirectory());
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
			engine::Engine::Destroy();
			return 1;
		}

#pragma region Vulkan
		auto pVulkan = pEngine->initializeVulkan(pWindow->querySDLVulkanExtensions());
		auto renderer = graphics::VulkanRenderer(pVulkan, pWindow->createSurface().initialize(pVulkan));

		// Initialize settings
		renderer.setPhysicalDevicePreference(graphics::PhysicalDevicePreference()
																				 .addCriteriaDeviceType(vk::PhysicalDeviceType::eDiscreteGpu, 128)
																				 .addCriteriaQueueFamily(graphics::QueueFamily::eGraphics)
																				 .addCriteriaQueueFamily(graphics::QueueFamily::ePresentation)
																				 .addCriteriaDeviceFeature(graphics::PhysicalDeviceFeature::GeometryShader)
																				 .addCriteriaDeviceExtension(VK_KHR_SWAPCHAIN_EXTENSION_NAME)
																				 .addCriteriaSwapChain(graphics::PhysicalDevicePreference::PreferenceSwapChain::Type::eHasAnySurfaceFormat)
																				 .addCriteriaSwapChain(graphics::PhysicalDevicePreference::PreferenceSwapChain::Type::eHasAnyPresentationMode)
		);
		renderer.setLogicalDeviceInfo(graphics::LogicalDeviceInfo()
																	.addDeviceExtension(VK_KHR_SWAPCHAIN_EXTENSION_NAME)
																	.addQueueFamily(graphics::QueueFamily::eGraphics)
																	.addQueueFamily(graphics::QueueFamily::ePresentation)
																	.setValidationLayers(engine::Engine::VulkanValidationLayers)
		);
		renderer.setSwapChainInfo(graphics::SwapChainInfo()
															.addFormatPreference(vk::Format::eB8G8R8A8Srgb)
															.setColorSpace(vk::ColorSpaceKHR::eSrgbNonlinear)
															.addPresentModePreference(vk::PresentModeKHR::eMailbox)
															.addPresentModePreference(vk::PresentModeKHR::eFifo)
		);
		renderer.setImageViewInfo({
			vk::ImageViewType::e2D,
			{
				vk::ComponentSwizzle::eIdentity,
				vk::ComponentSwizzle::eIdentity,
				vk::ComponentSwizzle::eIdentity,
				vk::ComponentSwizzle::eIdentity
			}
															});

		// Initialize required api connections
		renderer.initializeDevices();

		// Create shaders
		auto shaderVertex = graphics::ShaderModule();
		shaderVertex.setStage(vk::ShaderStageFlagBits::eVertex);
		shaderVertex.setSource("shaders/triangle.vert");
		shaderVertex.setVertexDescription(
			{
				sizeof(Vertex),
				{
					{ "position", (ui32)offsetof(Vertex, position) },
					{ "color", (ui32)offsetof(Vertex, color) }
				}
			}
		);

		auto shaderFragment = graphics::ShaderModule();
		shaderFragment.setStage(vk::ShaderStageFlagBits::eFragment);
		shaderFragment.setSource("shaders/triangle.frag");
		renderer.addShader(vk::ShaderStageFlagBits::eVertex, &shaderVertex);
		renderer.addShader(vk::ShaderStageFlagBits::eFragment, &shaderFragment);

		// Initialize the rendering api connections
		renderer.createInputBuffers(sizeof(Vertex) * (ui32)vertices.size());
		renderer.writeVertexData(vertices);

		renderer.createRenderChain();
		renderer.finalizeInitialization();

		// Give the window its renderer
		pWindow->setRenderer(&renderer);
#pragma endregion

		pEngine->start();
		while (pEngine->isActive())
		{
			pEngine->update();
		}
		pEngine->joinThreads();

		// TODO: Headless https://github.com/temportalflux/ChampNet/blob/feature/final/ChampNet/ChampNetPluginTest/source/StateApplication.cpp#L61

		renderer.invalidate();

		engine::Engine::Get()->destroyWindow(pWindow);
		pWindow.reset();

		engine::Engine::Destroy();
	}

	engine::Engine::LOG_SYSTEM.close();
	return 0;
}