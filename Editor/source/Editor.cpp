#include "Editor.hpp"

#include "Engine.hpp"
#include "Window.hpp"
#include "commandlet/CommandletBuildAssets.hpp"
#include "gui/asset/EditorProject.hpp"

#include <memory>
#include "memory/MemoryChunk.hpp"
#include <examples/imgui_impl_sdl.h>
#include <examples/imgui_impl_vulkan.h>

Editor* Editor::EDITOR = nullptr;

Editor::Editor(std::unordered_map<std::string, ui64> memoryChunkSizes)
{
	assert(EDITOR == nullptr);
	EDITOR = this;

	auto pEngine = engine::Engine::Create(memoryChunkSizes);
	pEngine->setProject(pEngine->getMiscMemory()->make_shared<asset::Project>(asset::Project("Editor", TE_GET_VERSION(TE_MAKE_VERSION(0, 0, 1)))));
	
	auto guiMemorySize = memoryChunkSizes.find("gui");
	this->mpMemoryGui = guiMemorySize != memoryChunkSizes.end() ? memory::MemoryChunk::Create(guiMemorySize->second) : nullptr;

	this->registerAllCommandlets();
}

std::shared_ptr<memory::MemoryChunk> Editor::getMemoryGui() const
{
	return this->mpMemoryGui;
}

Editor::~Editor()
{
	EDITOR = nullptr;
	this->mpDockspace.reset();
	this->mpMemoryGui.reset();
	this->mpProject.reset();
	this->mCommandlets.clear();
	engine::Engine::Destroy();
}

void Editor::registerAllCommandlets()
{
	auto miscMemory = engine::Engine::Get()->getMiscMemory();
	this->registerCommandlet(miscMemory->make_shared<editor::CommandletBuildAssets>());
}

void Editor::registerCommandlet(std::shared_ptr<editor::Commandlet> cmdlet)
{
	assert(!cmdlet->getId().empty());
	this->mCommandlets.insert(std::make_pair(cmdlet->getId(), cmdlet));
}

bool Editor::setup(utility::ArgumentMap args)
{
	this->mbShouldRender = args.find("noui") == args.end();

	auto pEngine = engine::Engine::Get();
	if (!pEngine->initializeDependencies(this->mbShouldRender)) return false;
	if (this->mbShouldRender && !pEngine->setupVulkan()) return false;
	
	if (this->mbShouldRender)
	{
		assert(this->mpMemoryGui != nullptr);
		this->mpDockspace = this->mpMemoryGui->make_shared<gui::MainDockspace>("Editor::MainDockspace", "Editor");
		this->mpDockspace->open();
		this->registerAllAssetEditors();
	}
	
	return true;
}

void Editor::registerAllAssetEditors()
{
	this->registerAssetEditor({ AssetType_Project, &gui::EditorProject::create });
}

void Editor::registerAssetEditor(RegistryEntryAssetEditor entry)
{
	assert(!this->hasRegisteredAssetEditor(entry.key));
	this->mAssetEditors.insert(std::make_pair(entry.key, entry));
}

bool Editor::hasRegisteredAssetEditor(asset::AssetType type) const
{
	return this->mAssetEditors.find(type) != this->mAssetEditors.end();
}

void Editor::run(utility::ArgumentMap args)
{
	if (!this->mbShouldRender)
	{
		// Running a headless command
		DeclareLog("Editor").log(logging::ECategory::LOGINFO, "no u(i)");

		std::string cmdletPrefix("cmdlet-");
		utility::ArgumentMap cmdlets = utility::getArgumentsWithPrefix(args, cmdletPrefix);
		for (const auto& [cmdletId, _] : cmdlets)
		{
			auto cmdletIter = this->mCommandlets.find(cmdletId);
			if (cmdletIter != this->mCommandlets.end())
			{
				cmdletIter->second->initialize(utility::getArgumentsWithPrefix(args, cmdletPrefix + cmdletId + "-"));
				cmdletIter->second->run();
			}
		}
		return;
	}

	this->mpEngine = engine::Engine::Get();

	this->mpWindow = this->mpEngine->createWindow(800, 600, "Editor", WindowFlags::RESIZABLE);
	if (this->mpWindow == nullptr) return;
	
	auto pVulkan = this->mpEngine->initializeVulkan(this->mpWindow->querySDLVulkanExtensions());
	this->mpRenderer = this->mpMemoryGui->make_shared<graphics::ImGuiRenderer>(
		pVulkan, this->mpWindow->createSurface().initialize(pVulkan)
	);
	this->initializeRenderer(this->mpRenderer);
	this->mpRenderer->finalizeInitialization();

	// TODO: window should take a shared pointer to the renderer
	this->mpWindow->setRenderer(this->mpRenderer.get());

	this->mpRenderer->addGui("MainDockspace", this->mpDockspace);

	this->mpEngine->start();
	while (this->mpEngine->isActive() && !this->mpWindow->isPendingClose() && this->mpDockspace->isOpen())
	{
		this->mpEngine->update();
	}
	this->mpEngine->joinThreads();

	this->mpRenderer->removeGui("MainDockspace");

	this->mpRenderer->invalidate();
	this->mpRenderer.reset();

	this->mpEngine->destroyWindow(this->mpWindow);
	this->mpEngine.reset();
}

void Editor::initializeRenderer(std::shared_ptr<graphics::VulkanRenderer> pRenderer)
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

bool Editor::hasProject() const
{
	return (bool)this->mpProject;
}

void Editor::setProject(asset::AssetPtrStrong asset)
{
	asset::ProjectPtrStrong project = std::dynamic_pointer_cast<asset::Project>(asset);
	assert(project != nullptr);
	this->mpProject = project;

	if (this->mpWindow)
	{
		this->mpWindow->setTitle("Editor: " + this->getProject()->getDisplayName());
	}
	
	asset::AssetManager::get()->scanAssetDirectory(this->mpProject->getAssetDirectory(), asset::EAssetSerialization::Json);
}

asset::ProjectPtrStrong Editor::getProject() const
{
	return this->mpProject;
}

std::filesystem::path Editor::getCurrentAssetDirectory() const
{
	return this->getProject()->getAssetDirectory();
}

void Editor::openAssetEditor(asset::AssetPtrStrong asset)
{
	DeclareLog("Editor").log(LOG_DEBUG, "Opening Asset Editor for %s", asset->getFileName().c_str());
	if (!this->hasRegisteredAssetEditor(asset->getAssetType()))
	{
		DeclareLog("Editor").log(LOG_DEBUG, "Cannot open editor, no editor registered for asset type %s", asset->getAssetType().c_str());
		return;
	}
	auto factory = this->mAssetEditors.find(asset->getAssetType())->second;
	auto editor = factory.create(engine::Engine::Get()->getAssetManager()->getAssetMemory());
	editor->setAsset(asset);
	this->mpRenderer->addGui(asset->getPath().string(), editor);
	editor->open();
}

void Editor::closeGui(std::string id)
{
	this->mpRenderer->removeGui(id);
}

void Editor::openProjectSettings()
{
	this->openAssetEditor(this->getProject());
}
