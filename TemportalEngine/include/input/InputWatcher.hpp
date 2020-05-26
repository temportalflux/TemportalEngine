#ifndef INPUT_WATCHER_HPP
#define INPUT_WATCHER_HPP


#include "TemportalEnginePCH.hpp"

#include "input/Event.hpp"
#include "types/integer.h"

#include <functional>

NS_INPUT

class TEMPORTALENGINE_API InputWatcher
{
public:
	typedef std::function<void(Event const &evt)> EventDelegateInput;
	typedef std::function<void(ui32 windowId, void* sdlEvent)> EventDelegateWindow;

private:

	static ui8 const MAX_JOYSTICK_COUNT = 8;

private:

	ui8 mJoystickCount;
	void* mpJoystickHandles[MAX_JOYSTICK_COUNT];

	EventDelegateInput mDelegateInput;
	EventDelegateWindow mDelegateWindow;

	void processEvent(void* evt);

public:
	InputWatcher();
	~InputWatcher();

	void setInputEventCallback(EventDelegateInput callback);
	void setWindowEventCallback(EventDelegateWindow callback);
	
	void initializeJoysticks(ui8 count);
	void pollInput();

};

NS_END

#endif