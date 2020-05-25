#include <Window.hpp>

#include <functional>
#include <SDL.h>
#include <SDL_syswm.h>
#include <SDL_vulkan.h>

#include "logging/Logger.hpp"
#include "Engine.hpp"
#include "graphics/VulkanRenderer.hpp"
#include "input/Queue.hpp"

#include "ExecutableInfo.hpp"

auto LogWindow = DeclareLog("Window");

Window::Window(ui16 width, ui16 height)
	: mWidth(width)
	, mHeight(height)
	, mpTitle("TODO Update Title")
{
	this->mIsPendingClose = false;

	this->mpHandle = SDL_CreateWindow(
		mpTitle, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		(int)mWidth, (int)mHeight,
		SDL_WINDOW_VULKAN
	);
	if (!this->isValid())
	{
		LogWindow.log(logging::ECategory::LOGERROR, "Failed to create window");
		return;
	}
	
	/*
	this->mpRenderer = engine::Engine::Get()->alloc<render::Renderer>(
		appInfo, engine::Engine::Get()->getInfo(),
		width, height,
		vulkanExtensionsForSDL,
		[&](VkInstance const *pInst, VkSurfaceKHR *pOutSurface) {
			if (!SDL_Vulkan_CreateSurface((SDL_Window*)mpHandle, *pInst, pOutSurface))
			{
				LogWindow.log(logging::ECategory::LOGERROR, "Failed to create SDL Vulkan surface");
				return false;
			}
			return true;
		}
	);
	//*/

}

std::vector<const char*> Window::querySDLVulkanExtensions() const
{
	ui32 extCount = 0;
	std::vector<const char*> vulkanExtensionsForSDL = {};
	SDL_Window *pHandle = reinterpret_cast<SDL_Window*>(this->mpHandle);
	auto result = SDL_Vulkan_GetInstanceExtensions(pHandle, &extCount, NULL);
	if (!result)
	{
		//LogWindow.log(logging::ECategory::LOGERROR, "SDL-Vulkan: Failed to get required vulkan extension count.");
	}
	else
	{
		//LogWindow.log(logging::ECategory::LOGDEBUG, "SDL-Vulkan: Found %i extensions", extCount);
		vulkanExtensionsForSDL.resize(extCount);
		if (!SDL_Vulkan_GetInstanceExtensions(pHandle, &extCount, vulkanExtensionsForSDL.data()))
		{
			//LogWindow.log(logging::ECategory::LOGERROR, "Failed to get required vulkan extensions for SDL.");
		}
	}
	return vulkanExtensionsForSDL;
}

graphics::Surface Window::createSurface() const
{
	return graphics::Surface(this->mpHandle);
}

void Window::setRenderer(graphics::VulkanRenderer *pRenderer)
{
	mpRenderer = pRenderer;
}

void Window::destroy()
{
	// TODO: Undo input listeners
	if (this->mpHandle != nullptr)
	{
		SDL_DestroyWindow((SDL_Window*)this->mpHandle);
		LogWindow.log(logging::ECategory::LOGDEBUG, "Destroyed window. Errors? %s", SDL_GetError());
		this->mpHandle = nullptr;
	}
}

bool Window::isValid()
{
	return this->mpHandle != nullptr;
}

void Window::addInputListeners(input::Queue *pQueue)
{
	mInputHandleQuit = pQueue->addListener(input::EInputType::QUIT,
		std::bind(&Window::onInputQuit, this, std::placeholders::_1)
	);
}

void Window::onInputQuit(input::Event const &evt)
{
	markShouldClose();
}

void Window::markShouldClose()
{
	this->mIsPendingClose = true;
}

bool Window::isPendingClose()
{
	return this->mIsPendingClose;
}

bool Window::renderUntilClose()
{
	mpRenderer->drawFrame();
	return this->isValid() && !this->isPendingClose();
}

void Window::waitForCleanup()
{
	mpRenderer->waitUntilIdle();
}
