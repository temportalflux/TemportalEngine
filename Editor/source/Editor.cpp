#include "Editor.hpp"

#include "Engine.hpp"
#include "Window.hpp"
#include "graphics/ImGuiRenderer.hpp"

Editor::Editor()
{
	std::string logFileName = "TemportalEngine_Editor_" + logging::LogSystem::getCurrentTimeString() + ".log";
	engine::Engine::LOG_SYSTEM.open(logFileName.c_str());

	auto pEngine = engine::Engine::Create();

	utility::SExecutableInfo appInfo = { "Editor", TE_MAKE_VERSION(0, 0, 1) };
	pEngine->setApplicationInfo(&appInfo);
}

Editor::~Editor()
{
	engine::Engine::Destroy();
	engine::Engine::LOG_SYSTEM.close();
}

bool Editor::setup()
{
	auto pEngine = engine::Engine::Get();
	if (!pEngine->initializeDependencies()) return false;
	if (!pEngine->setupVulkan()) return false;
	return true;
}

void Editor::run()
{
	auto pEngine = engine::Engine::Get();

	auto pWindow = pEngine->createWindow(800, 600);
	if (pWindow == nullptr) return;
	
	// In original code, this was before vulkan initialization. Does it work if its after the vulkan instance? Currently lives inside ImGuiRenderer
	//ImGui_ImplSDL2_InitForVulkan(reinterpret_cast<SDL_Window*>(handle));

	auto pVulkan = pEngine->initializeVulkan(pWindow->querySDLVulkanExtensions());
	auto renderer = graphics::ImGuiRenderer(pVulkan, pWindow->createSurface().initialize(pVulkan));
	this->initializeRenderer(&renderer);
	renderer.finalizeInitialization();

	pWindow->setRenderer(&renderer);

	pEngine->run(pWindow);

	renderer.invalidate();
	pEngine->destroyWindow(pWindow);
}

void Editor::initializeRenderer(graphics::VulkanRenderer *pRenderer)
{
	pRenderer->setPhysicalDevicePreference(
		graphics::PhysicalDevicePreference()
		.addCriteriaQueueFamily(graphics::QueueFamily::eGraphics)
		.addCriteriaQueueFamily(graphics::QueueFamily::ePresentation)
	);
	pRenderer->setLogicalDeviceInfo(
		graphics::LogicalDeviceInfo()
		.addQueueFamily(graphics::QueueFamily::eGraphics)
		.addQueueFamily(graphics::QueueFamily::ePresentation)
		.addDeviceExtension(VK_KHR_SWAPCHAIN_EXTENSION_NAME)
		.setValidationLayers(engine::Engine::VulkanValidationLayers)
	);
	pRenderer->setSwapChainInfo(
		graphics::SwapChainInfo()
		.addFormatPreference(vk::Format::eB8G8R8A8Unorm)
		.addFormatPreference(vk::Format::eR8G8B8A8Unorm)
		.addFormatPreference(vk::Format::eB8G8R8Unorm)
		.addFormatPreference(vk::Format::eR8G8B8Unorm)
		.setColorSpace(vk::ColorSpaceKHR::eSrgbNonlinear)
#ifdef IMGUI_UNLIMITED_FRAME_RATE
		.addPresentModePreference(vk::PresentModeKHR::eMailbox)
		.addPresentModePreference(vk::PresentModeKHR::eImmediate)
#endif
		.addPresentModePreference(vk::PresentModeKHR::eFifo)
	);
	pRenderer->setImageViewInfo({
		vk::ImageViewType::e2D,
		{
			vk::ComponentSwizzle::eR,
			vk::ComponentSwizzle::eG,
			vk::ComponentSwizzle::eB,
			vk::ComponentSwizzle::eA
		}
	});

	pRenderer->initializeDevices();
	pRenderer->createRenderChain();
}
