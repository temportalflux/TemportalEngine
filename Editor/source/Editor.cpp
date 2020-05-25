#include "Editor.hpp"

#include "types/real.h"
#include "ExecutableInfo.hpp"
#include "version.h"

#include <time.h>
#include <SDL.h>
#include <SDL_vulkan.h>

SDL_Window* GetSDLWindow(void* handle)
{
	return reinterpret_cast<SDL_Window*>(handle);
}

std::vector<const char*> Editor::VulkanValidationLayers = { "VK_LAYER_KHRONOS_validation" };

Editor::Editor()
	: LogSystem(logging::LogSystem())
	, mIsRunning(true)
{
	// TODO: Unify versions in a header/config file
	utility::SExecutableInfo appInfo = { "Editor", TE_MAKE_VERSION(0, 0, 1) };
	utility::SExecutableInfo engineInfo = { "TemportalEngine", TE_MAKE_VERSION(0, 0, 1) };
	std::string logFileName = "";
	logFileName += engineInfo.title;
	logFileName += "_";
	logFileName += appInfo.title;
	logFileName += "_";
	logFileName += logging::LogSystem::getCurrentTimeString();
	logFileName += ".log";
	this->LogSystem.open(logFileName.c_str());

	this->initializeDependencies();

	this->mVulkanInstance.setApplicationInfo(appInfo);
	this->mVulkanInstance.setEngineInfo(engineInfo);
	this->mVulkanInstance.createLogger(&this->LogSystem, /*vulkan debug*/ true);

}

Editor::~Editor()
{
	this->closeWindow();
	if (this->mVulkanInstance.isValid())
	{
		this->mVulkanInstance.destroy();
	}

	this->terminateDependencies();
	this->LogSystem.close();
}

bool Editor::initializeDependencies()
{
	if (!mDependencySDL->initialize()) return false;
	return true;
}

void Editor::terminateDependencies()
{
	if (mDependencySDL->isInitialized())
	{
		mDependencySDL->terminate();
	}
}

void Editor::openWindow()
{
	this->createWindow();
	
	// Initialize the vulkan instance
	this->mVulkanInstance.setRequiredExtensions(this->querySDLVulkanExtensions());
	this->mVulkanInstance.setValidationLayers(VulkanValidationLayers);
	this->mVulkanInstance.initialize();

	this->createGui();
}

void Editor::closeWindow()
{
	if (this->mpWindowHandle != nullptr)
	{
		this->mGui.destroy(&this->mVulkanInstance);
	}
	this->destroyWindow();
}

void Editor::createWindow()
{
	// TODO: Allow resizing, but that means re-creating the swapchain when it happens
	auto flags = (SDL_WindowFlags)(SDL_WINDOW_VULKAN);// | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
	this->mpWindowHandle = SDL_CreateWindow("TemportalEngine Editor",
		SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		1280, 720,
		flags
	);
}

void Editor::destroyWindow()
{
	if (this->mpWindowHandle != nullptr)
	{
		SDL_DestroyWindow(GetSDLWindow(this->mpWindowHandle));
		this->mpWindowHandle = nullptr;
	}
}

std::vector<const char*> Editor::querySDLVulkanExtensions() const
{
	ui32 extCount = 0;
	std::vector<const char*> vulkanExtensionsForSDL = {};
	auto result = SDL_Vulkan_GetInstanceExtensions(GetSDLWindow(this->mpWindowHandle), &extCount, NULL);
	if (!result)
	{
		//LogWindow.log(logging::ECategory::LOGERROR, "SDL-Vulkan: Failed to get required vulkan extension count.");
	}
	else
	{
		//LogWindow.log(logging::ECategory::LOGDEBUG, "SDL-Vulkan: Found %i extensions", extCount);
		vulkanExtensionsForSDL.resize(extCount);
		if (!SDL_Vulkan_GetInstanceExtensions(GetSDLWindow(this->mpWindowHandle), &extCount, vulkanExtensionsForSDL.data()))
		{
			//LogWindow.log(logging::ECategory::LOGERROR, "Failed to get required vulkan extensions for SDL.");
		}
	}
	return vulkanExtensionsForSDL;
}

void Editor::createGui()
{
	this->mGui.initContext();
	this->mGui.initWindow(this->mpWindowHandle);
	this->mGui.initVulkan(&this->mVulkanInstance);
	this->mGui.submitFonts();
}

void Editor::run()
{
	while (mIsRunning)
	{
		this->pollInput();
	}
}

void Editor::pollInput()
{
	SDL_Event event;
	while (SDL_PollEvent(&event))
	{
		this->mGui.processInput(&event);
		if (event.type == SDL_QUIT)
		{
			mIsRunning = false;
		}
		/*
		if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_RESIZED && event.window.windowID == SDL_GetWindowID(window))
		{
			g_SwapChainResizeWidth = (int)event.window.data1;
			g_SwapChainResizeHeight = (int)event.window.data2;
			g_SwapChainRebuild = true;
		}
		//*/
	}
}
