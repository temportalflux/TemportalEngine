#include <Window.hpp>

#include <functional>
#include <SDL.h>
#include <SDL_syswm.h>
#include <SDL_vulkan.h>

#include "logging/Logger.hpp"
#include "Engine.hpp"
#include "render/Renderer.hpp"

#include "ExecutableInfo.hpp"

void Window::renderUntilClose(void* ptr)
{
	Window* pWindow = reinterpret_cast<Window*>(ptr);
	while (pWindow->isValid())
	{
		pWindow->render();
	}
}

Window::Window(uSize width, uSize height, utility::SExecutableInfo const *const appInfo)
	: mWidth(width)
	, mHeight(height)
	, mpTitle(appInfo->title)
{
	this->mIsPendingClose = false;

	this->mpHandle = SDL_CreateWindow(mpTitle, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, (int)mWidth, (int)mHeight, 0);
	if (!this->isValid())
	{
		DeclareLog("Window").log(logging::ECategory::LOGERROR, "Failed to create window");
		return;
	}

	SDL_SysWMinfo info;
	SDL_VERSION(&info.version);
	SDL_GetWindowWMInfo((SDL_Window*)this->mpHandle, &info);
	void* windowHandle_win32 = info.info.win.window;
	void* applicationHandle_win32 = GetModuleHandle(NULL);

	ui32 extCount;
	std::vector<const char*> vulkanExtensionsForSDL = {};
	if (!SDL_Vulkan_GetInstanceExtensions((SDL_Window*)this->mpHandle, &extCount, vulkanExtensionsForSDL.data()))
	{
		throw std::runtime_error("Failed to get required vulkan extensions for SDL.");
	}

	this->mpRenderer = engine::Engine::Get()->alloc<render::Renderer>(
		applicationHandle_win32, windowHandle_win32,
		appInfo, engine::Engine::Get()->getInfo(),
		vulkanExtensionsForSDL
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
	this->mpRenderer->initializeWindow();
}

void Window::render()
{
	this->mpRenderer->render();
}
