#include <Window.hpp>

#include <functional>
#include <SDL.h>

#include "logging/Logger.hpp"
#include "Engine.hpp"

void Window::renderUntilClose(void* ptr)
{
	Window* pWindow = reinterpret_cast<Window*>(ptr);
	while (pWindow->isValid())
	{
		pWindow->render();
	}
}

Window::Window(uSize width, uSize height, char const * title)
	: mWidth(width)
	, mHeight(height)
	, mpTitle(title)
{
	this->mIsPendingClose = false;

	this->mpHandle = SDL_CreateWindow(mpTitle, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, (int)mWidth, (int)mHeight, 0);
	if (!this->isValid())
	{
		DeclareLog("Window").log(logging::ECategory::LOGERROR,
				"Failed to create window");
		return;
	}

}

void Window::destroy()
{
	
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

void Window::initializeRenderContext(int bufferSwapInterval)
{
	//glfwMakeContextCurrent(this->mpHandle);
	//gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);
	//glfwSwapInterval(bufferSwapInterval);
}

void Window::render()
{
	//glfwSwapBuffers(this->mpHandle);
}
