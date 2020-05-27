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
	ui16 width, ui16 height, WindowFlags flags
)
	: mWidth(width)
	, mHeight(height)
	, mpTitle("TODO Update Title")
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

void Window::onEvent(void* pSdlEvent)
{
	SDL_Event *evt = reinterpret_cast<SDL_Event*>(pSdlEvent);
	if (evt->type == SDL_WINDOWEVENT && evt->window.windowID == this->getId())
	{
		switch (evt->window.event)
		{
		case SDL_WINDOWEVENT_RESIZED:
		{
			this->mpRenderer->markRenderChainDirty();
			break;
		}
		case SDL_WINDOWEVENT_CLOSE:
			markShouldClose();
			break;
		default: break;
		}
	}
	this->mpRenderer->onInputEvent(pSdlEvent);
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

	this->mpRenderer->update();

	if (!this->hasFlag(WindowFlags::RENDER_ON_THREAD))
	{
		this->renderUntilClose();
	}
	return true;
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
