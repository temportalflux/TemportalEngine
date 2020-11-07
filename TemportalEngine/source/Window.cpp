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

Window::Window(
	ui16 width, ui16 height, std::string title, WindowFlags flags
)
	: mWidth(width)
	, mHeight(height)
	, mpTitle(title)
	, mFlags(flags)
{
	this->mIsPendingClose = false;

	ui32 windowFlags = SDL_WINDOW_VULKAN;
	if (this->hasFlag(WindowFlags::RESIZABLE)) windowFlags |= SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI;
	if (this->hasFlag(WindowFlags::BORDERLESS)) windowFlags |= SDL_WINDOW_BORDERLESS;

	this->mpHandle = SDL_CreateWindow(
		mpTitle.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		(int)mWidth, (int)mHeight, windowFlags
	);
	if (!this->isValid())
	{
		LogWindow.log(logging::ECategory::LOGERROR, "Failed to create window");
		return;
	}
	this->mId = SDL_GetWindowID(reinterpret_cast<SDL_Window*>(this->mpHandle));
}

bool Window::hasFlag(WindowFlags flag) const
{
	return ((ui32)this->mFlags & (ui32)flag) != 0;
}

ui32 Window::getId() const
{
	return this->mId;
}

void* Window::getWindowHandle() const
{
	return this->mpHandle;
}

void Window::setTitle(std::string title)
{
	this->mpTitle = title;
	SDL_SetWindowTitle(reinterpret_cast<SDL_Window*>(this->getWindowHandle()), this->mpTitle.c_str());
}

void Window::showCursor(bool show)
{
	SDL_ShowCursor(show ? SDL_ENABLE : SDL_DISABLE);
}

void Window::consumeCursor(bool consume)
{
	SDL_SetRelativeMouseMode(consume ? SDL_TRUE : SDL_FALSE);
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

void Window::addInputListeners(std::shared_ptr<input::Queue> pQueue)
{
	pQueue->OnInputEvent.bind(
		input::EInputType::QUIT, this->weak_from_this(),
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

void Window::onEvent(void* pSdlEvent)
{
	SDL_Event *evt = reinterpret_cast<SDL_Event*>(pSdlEvent);
	if (evt->type == SDL_WINDOWEVENT && evt->window.windowID == this->getId())
	{
		switch (evt->window.event)
		{
		case SDL_WINDOWEVENT_RESIZED:
		{
			i32 w, h;
			SDL_GetWindowSize(reinterpret_cast<SDL_Window*>(this->getWindowHandle()), &w, &h);
			this->mWidth = (ui16)w;
			this->mHeight = (ui16)h;

			this->mpRenderer->markRenderChainDirty();
			break;
		}
		case SDL_WINDOWEVENT_CLOSE:
			markShouldClose();
			break;
		default: break;
		}
	}
	if (this->mpRenderer)
	{
		this->mpRenderer->onInputEvent(pSdlEvent);
	}
}

void Window::startThread()
{
	if (!this->hasFlag(WindowFlags::RENDER_ON_THREAD)) return;
	this->mRenderThread = Thread("Thread:" + this->mpTitle, &engine::Engine::LOG_SYSTEM);
	this->mRenderThread.setFunctor(std::bind(&Window::renderUntilClose, this));
	this->mRenderThread.setOnComplete(std::bind(&Window::waitForCleanup, this));
	this->mRenderThread.start();
}

void Window::joinThread()
{
	if (this->mRenderThread.isValid())
	{
		this->mRenderThread.join();
	}
	else
	{
		this->waitForCleanup();
	}
}

bool Window::update()
{
	if (!this->isValid() || this->isPendingClose()) return false;

	if (this->mpRenderer)
	{
		this->mpRenderer->update();
	}

	if (!this->hasFlag(WindowFlags::RENDER_ON_THREAD))
	{
		this->renderUntilClose();
	}
	return true;
}

bool Window::renderUntilClose()
{
	if (this->mpRenderer)
	{
		this->mpRenderer->drawFrame();
	}
	return this->isValid() && !this->isPendingClose();
}

void Window::waitForCleanup()
{
	if (this->mpRenderer)
	{
		this->mpRenderer->waitUntilIdle();
	}
}
