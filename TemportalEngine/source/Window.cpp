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

inline input::EAction getActionFromSDLKey(SDL_Event const evtIn)
{
	if (evtIn.type == SDL_KEYUP)
		return input::EAction::RELEASE;
	else if (evtIn.key.repeat > 0)
		return input::EAction::REPEAT;
	else
		return input::EAction::PRESS;
}

inline void makeEventKey(SDL_Event const evtIn, input::Event &evtOut)
{
	evtOut.type = input::EInputType::KEY;
	evtOut.inputKey.action = getActionFromSDLKey(evtIn);
	evtOut.inputKey.modifierMask = (input::EKeyModifier)evtIn.key.keysym.mod;
	evtOut.inputKey.key = (input::EKey)evtIn.key.keysym.scancode;
}


inline void makeEventMouseButton(SDL_Event const evtIn, input::Event &evtOut)
{
	evtOut.type = input::EInputType::MOUSE_BUTTON;
	evtOut.inputMouseButton.action = evtIn.type == SDL_MOUSEBUTTONDOWN ? input::EAction::PRESS : input::EAction::RELEASE;
	evtOut.inputMouseButton.button = (input::EMouseButton)evtIn.button.button;
	evtOut.inputMouseButton.clickCount = evtIn.button.clicks;
	evtOut.inputMouseButton.xCoord = evtIn.button.x;
	evtOut.inputMouseButton.yCoord = evtIn.button.y;
}

bool makeInputEvent(SDL_Event const evtIn, input::Event &evtOut)
{
	switch (evtIn.type)
	{
	case SDL_KEYDOWN:
	case SDL_KEYUP:
		makeEventKey(evtIn, evtOut);
		return true;
	case SDL_MOUSEMOTION:
	{
		evtOut.type = input::EInputType::MOUSE_MOVE;
		evtOut.inputMouseMove.xCoord = evtIn.motion.x;
		evtOut.inputMouseMove.yCoord = evtIn.motion.y;
		evtOut.inputMouseMove.xDelta = evtIn.motion.xrel;
		evtOut.inputMouseMove.yDelta = evtIn.motion.yrel;
		return true;
	}
	case SDL_MOUSEBUTTONDOWN:
	case SDL_MOUSEBUTTONUP:
		makeEventMouseButton(evtIn, evtOut);
		return true;
	case SDL_MOUSEWHEEL:
	{
		evtOut.type = input::EInputType::MOUSE_SCROLL;
		i8 directionMultiplier = evtIn.wheel.direction == SDL_MOUSEWHEEL_FLIPPED ? -1 : 1;
		evtOut.inputScroll.xDelta = evtIn.wheel.x * directionMultiplier;
		evtOut.inputScroll.yDelta = evtIn.wheel.y * directionMultiplier;
		return true;
	}
	case SDL_QUIT:
		evtOut.type = input::EInputType::QUIT;
		return true;
	default:
		return false;
	}
}

void Window::pollInput()
{
	SDL_Event sdlEvent;
	input::Event evt;
	while (SDL_PollEvent(&sdlEvent))
	{
		if (makeInputEvent(sdlEvent, evt))
			this->executeInputCallback(evt);
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
