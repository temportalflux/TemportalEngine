#include "input/InputWatcher.hpp"

#include <SDL.h>

using namespace input;

InputWatcher::InputWatcher(std::optional<EventDelegateRaw> rawCallback)
{
	mDelegateRaw = rawCallback;
	mJoystickCount = 0;
}

InputWatcher::~InputWatcher()
{
	for (ui8 i = 0; i < mJoystickCount; ++i)
		SDL_JoystickClose((SDL_Joystick*)mpJoystickHandles[i]);
}

void InputWatcher::setInputEventCallback(EventDelegateInput callback)
{
	this->mDelegateInput = callback;
}

void InputWatcher::initializeJoysticks(ui8 count)
{
	SDL_JoystickEventState(SDL_ENABLE);

	mJoystickCount = count;

	// https://www.libsdl.org/release/SDL-1.2.15/docs/html/guideinput.html
	for (ui8 i = 0; i < mJoystickCount; ++i)
		mpJoystickHandles[i] = SDL_JoystickOpen(i);
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

#pragma region Keyboard
	case SDL_KEYDOWN:
	case SDL_KEYUP:
		makeEventKey(evtIn, evtOut);
		return true;
#pragma endregion

#pragma region Mouse
	case SDL_MOUSEMOTION:
	{
		i32 width, height;
		SDL_GetWindowSize(SDL_GetWindowFromID(evtIn.motion.windowID), &width, &height);

		evtOut.type = input::EInputType::MOUSE_MOVE;
		evtOut.inputMouseMove.xCoord = evtIn.motion.x;
		evtOut.inputMouseMove.yCoord = evtIn.motion.y;
		evtOut.inputMouseMove.xDelta = evtIn.motion.xrel / (f32)width;
		evtOut.inputMouseMove.yDelta = evtIn.motion.yrel / (f32)height;
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
#pragma endregion

#pragma region Joystick
	case SDL_JOYDEVICEADDED:
		//DeclareLog("Window").log(logging::ECategory::DEBUG, "Joystick added");
		break;
	case SDL_JOYDEVICEREMOVED:
		//DeclareLog("Window").log(logging::ECategory::DEBUG, "Joystick removed");
		break;
	case SDL_JOYAXISMOTION:
	case SDL_JOYBUTTONDOWN:
	case SDL_JOYBUTTONUP:
		//DeclareLog("Window").log(logging::ECategory::DEBUG, "Joystick input");
		break;
#pragma endregion

#pragma region Controller
	case SDL_CONTROLLERDEVICEADDED:
		//DeclareLog("Window").log(logging::ECategory::DEBUG, "Controller added");
		break;
	case SDL_CONTROLLERDEVICEREMOVED:
		//DeclareLog("Window").log(logging::ECategory::DEBUG, "Controller removed");
		break;
	case SDL_CONTROLLERDEVICEREMAPPED:
	case SDL_CONTROLLERAXISMOTION:
	case SDL_CONTROLLERBUTTONDOWN:
	case SDL_CONTROLLERBUTTONUP:
		//DeclareLog("Window").log(logging::ECategory::DEBUG, "Controller input");
		break;
#pragma endregion


#pragma region Other
	case SDL_QUIT:
		evtOut.type = input::EInputType::QUIT;
		return true;
#pragma endregion

	default:
		return false;
	}

	return false;
}

void InputWatcher::pollInput()
{
	SDL_Event sdlEvent;
	while (SDL_PollEvent(&sdlEvent))
	{
		this->processEvent(&sdlEvent);
	}
}

void InputWatcher::processEvent(void* pEvt)
{
	SDL_Event *evt = reinterpret_cast<SDL_Event*>(pEvt);
	if (this->mDelegateRaw.has_value())
	{
		this->mDelegateRaw.value()(pEvt);
	}
	if (this->mDelegateInput)
	{
		input::Event inputEvent;
		if (makeInputEvent(*evt, inputEvent))
		{
			this->mDelegateInput(inputEvent);
		}
	}
}
