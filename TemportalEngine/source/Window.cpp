#include <Window.hpp>

#include <functional>
#include <SDL.h>
#include <SDL_syswm.h>
#include <SDL_vulkan.h>

#include "logging/Logger.hpp"
#include "Engine.hpp"
#include "render/Renderer.hpp"

#include "ExecutableInfo.hpp"

auto LogWindow = DeclareLog("Window");

void Window::renderUntilClose(void* ptr)
{
	Window* pWindow = reinterpret_cast<Window*>(ptr);
	while (pWindow->isValid())
	{
		pWindow->render();
	}
}

Window::Window(ui32 width, ui32 height, utility::SExecutableInfo const *const appInfo)
	: mWidth(width)
	, mHeight(height)
	, mpTitle(appInfo->title)
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

	SDL_SysWMinfo info;
	SDL_VERSION(&info.version);
	if (!SDL_GetWindowWMInfo((SDL_Window*)this->mpHandle, &info))
	{
		LogWindow.log(logging::ECategory::LOGERROR, "Failed to fetch SDL window info");
	}
	void* windowHandle_win32 = info.info.win.window;
	void* applicationHandle_win32 = GetModuleHandle(NULL);

	ui32 extCount = 0;
	std::vector<const char*> vulkanExtensionsForSDL = {};
	auto result = SDL_Vulkan_GetInstanceExtensions((SDL_Window*)this->mpHandle, &extCount, NULL);
	if (!result)
	{
		LogWindow.log(logging::ECategory::LOGERROR, "SDL-Vulkan: Failed to get required vulkan extension count.");
	}
	else
	{
		LogWindow.log(logging::ECategory::LOGDEBUG, "SDL-Vulkan: Found %i extensions", extCount);
		vulkanExtensionsForSDL.resize(extCount);
		if (!SDL_Vulkan_GetInstanceExtensions((SDL_Window*)this->mpHandle, &extCount, vulkanExtensionsForSDL.data()))
		{
			LogWindow.log(logging::ECategory::LOGERROR, "Failed to get required vulkan extensions for SDL.");
		}
	}

	this->mpRenderer = engine::Engine::Get()->alloc<render::Renderer>(
		appInfo, engine::Engine::Get()->getInfo(),
		width, height,
		vulkanExtensionsForSDL,
		[&](VkInstance const *pInst, VkSurfaceKHR *pOutSurface) {
			if (!SDL_Vulkan_CreateSurface((SDL_Window*)mpHandle, *pInst, pOutSurface))
			{
				LogWindow.log(logging::ECategory::LOGERROR, "Failed to create SDL Vulkan surface");
			}
		}
	);

}

void Window::destroy()
{

	engine::Engine::Get()->dealloc(&mpRenderer);

	if (this->mpHandle != nullptr)
	{
		SDL_DestroyWindow((SDL_Window*)this->mpHandle);
		this->mpHandle = nullptr;
	}

}

bool Window::isValid()
{
	return this->mpHandle != nullptr && !this->isPendingClose();
}

void Window::markShouldClose()
{
	this->mIsPendingClose = true;
}

bool Window::isPendingClose()
{
	return this->mIsPendingClose;
}

void Window::initializeRenderContext()
{
}

void Window::render()
{
}
