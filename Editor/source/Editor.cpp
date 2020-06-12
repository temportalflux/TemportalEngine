#include "Editor.hpp"

#include "Engine.hpp"
#include "Window.hpp"
#include "asset/AssetManager.hpp"
#include "asset/Shader.hpp"
#include "asset/Settings.hpp"
#include "commandlet/CommandletBuildAssets.hpp"
#include "gui/asset/EditorProject.hpp"
#include "gui/asset/EditorShader.hpp"
#include "gui/asset/EditorSettings.hpp"

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
	pEngine->getAssetManager()->registerType(
		AssetType_EditorSettings, CREATE_ASSETTYPE_METADATA(asset::Settings, "EditorSettings", ".settings", std::nullopt)
	);
	
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
	this->mAssetEditors.clear();
	this->mpEditorSettings.reset();
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
	this->registerAssetEditor({ AssetType_EditorSettings, &gui::EditorSettings::create });
	this->registerAssetEditor({ AssetType_Project, &gui::EditorProject::create });
	this->registerAssetEditor({ AssetType_Shader, &gui::EditorShader::create });
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

	this->mpWindow = this->mpEngine->createWindow(1280, 720, "Editor", WindowFlags::RESIZABLE);
	if (this->mpWindow == nullptr) return;
	
	this->mpRenderer = this->mpMemoryGui->make_shared<graphics::ImGuiRenderer>();
	auto pVulkan = this->mpEngine->initializeVulkan(this->mpWindow->querySDLVulkanExtensions());
	this->mpRenderer->setInstance(pVulkan);
	this->mpRenderer->takeOwnershipOfSurface(this->mpWindow->createSurface().initialize(pVulkan));
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
	this->mpWindow.reset();
	this->mpEngine.reset();
}

void Editor::initializeRenderer(std::shared_ptr<graphics::VulkanRenderer> pRenderer)
{
	pRenderer->setPhysicalDevicePreference(
		graphics::PhysicalDevicePreference()
		.addCriteriaQueueFamily(graphics::QueueFamily::Enum::eGraphics)
		.addCriteriaQueueFamily(graphics::QueueFamily::Enum::ePresentation)
	);
	pRenderer->setLogicalDeviceInfo(
		graphics::LogicalDeviceInfo()
		.addQueueFamily(graphics::QueueFamily::Enum::eGraphics)
		.addQueueFamily(graphics::QueueFamily::Enum::ePresentation)
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

#pragma region Project Being Editted

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

	this->mpEditorSettings.reset();
	this->loadEditorSettings(project->getAbsoluteDirectoryPath());
	
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

#pragma endregion

#pragma region Editor Settings per Project

void Editor::loadEditorSettings(std::filesystem::path projectDir)
{
	auto assetManager = asset::AssetManager::get();
	auto settingsPath = projectDir / "config" / ("Editor" + assetManager->getAssetTypeMetadata(AssetType_EditorSettings).fileExtension);
	std::filesystem::create_directories(settingsPath.parent_path());
	if (std::filesystem::exists(settingsPath))
		this->mpEditorSettings = asset::readFromDisk<asset::Settings>(settingsPath, asset::EAssetSerialization::Json, false);
	else
		this->mpEditorSettings = assetManager->createAssetAs<asset::Settings>(AssetType_EditorSettings, settingsPath);
}

std::shared_ptr<asset::Settings> Editor::getEditorSettings() const
{
	return this->mpEditorSettings;
}

std::filesystem::path Editor::getOutputDirectory() const
{
	assert(this->hasProject());
	assert(this->mpEditorSettings);
	return this->mpEditorSettings->getOutputDirectory();
}

std::filesystem::path Editor::getAssetBinaryPath(asset::AssetPtrStrong asset) const
{
	// Example:
	// AbsolutePath: asset->getPath() => "C:/Desktop/Engine/DemoGame/assets/shader/Shader1.te-asset"
	// AbsolutePath: projectDir => "C:/Desktop/Engine/DemoGame/"
	auto projectDir = this->getProject()->getAbsoluteDirectoryPath();
	// RelativePath: pathRelativeToProject = "assets/shader/Shader1.te-asset"
	auto pathRelativeToProject = std::filesystem::relative(asset->getPath(), projectDir);
	// RelativePath: this->getOutputDirectory() => "../output/DemoGame/"
	// MixedPath: pathInOutputDir = "C:/Desktop/Engine/DemoGame/../output/DemoGame/assets/shader/Shader1.te-asset"
	auto pathInOutputDir = projectDir / this->getOutputDirectory() / pathRelativeToProject;
	// Returns: "C:/Desktop/Engine/output/DemoGame/assets/shader/Shader1.te-asset"
	return std::filesystem::absolute(pathInOutputDir);
}

#pragma endregion

#pragma region View Management Shortcuts

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

void Editor::openSettings()
{
	this->openAssetEditor(this->getEditorSettings());
}

#pragma endregion
