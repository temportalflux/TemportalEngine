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
	this->mpHandle = SDL_CreateWindow(mpTitle, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, mWidth, mHeight, 0);
	if (!this->isValid())
	{
		DeclareLog("Window").log(logging::ECategory::ERROR,
				"Failed to create window");
	}
}

bool Window::isValid()
{
	return this->mpHandle != nullptr && !this->isPendingClose();
}

void Window::setInputCallback(DelegateKeyCallback callback)
{
	this->mpDelegateInputCallback = callback;
}

void Window::executeInputCallback(input::Event const &input)
{
	if (this->mpDelegateInputCallback != nullptr)
	{
		(*this->mpDelegateInputCallback)(this, input);
	}
}

void Window::markShouldClose()
{
	this->mIsPendingClose = true;
}

bool Window::isPendingClose()
{
	return this->mIsPendingClose;
}

void Window::pollInput()
{
	SDL_Event sdlEvent;
	input::Event evt;
	while (SDL_PollEvent(&sdlEvent))
	{
		switch (sdlEvent.type)
		{
		case SDL_KEYDOWN:
		case SDL_KEYUP:
		case SDL_MOUSEMOTION:
		case SDL_MOUSEBUTTONDOWN:
		case SDL_MOUSEBUTTONUP:
		case SDL_MOUSEWHEEL:
		{
			break;
		}
		case SDL_QUIT:
		{
			evt.type = input::EInputType::QUIT;
			this->executeInputCallback(evt);
			break;
		}
		default:
			break;
		}
	}
}

void Window::render()
{
	//glfwSwapBuffers(this->mpHandle);
}

void Window::destroy()
{
	SDL_DestroyWindow(this->mpHandle);
	this->mpHandle = nullptr;
}

void Window::initializeRenderContext(int bufferSwapInterval)
{
	//glfwMakeContextCurrent(this->mpHandle);
	//gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);
	//glfwSwapInterval(bufferSwapInterval);
}
