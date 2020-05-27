#ifndef INPUT_WATCHER_HPP
#define INPUT_WATCHER_HPP


#include "TemportalEnginePCH.hpp"

#include "input/Event.hpp"
#include "types/integer.h"

#include <functional>
#include <optional>

NS_INPUT

class TEMPORTALENGINE_API InputWatcher
{
public:
	typedef std::function<void(Event const &evt)> EventDelegateInput;
	typedef std::function<void(void* sdlEvent)> EventDelegateRaw;

private:

	static ui8 const MAX_JOYSTICK_COUNT = 8;

private:

	ui8 mJoystickCount;
	void* mpJoystickHandles[MAX_JOYSTICK_COUNT];

	std::optional<EventDelegateRaw> mDelegateRaw;
	EventDelegateInput mDelegateInput;

	void processEvent(void* evt);

public:
	InputWatcher(std::optional<EventDelegateRaw> rawCallback = std::nullopt);
	~InputWatcher();

	void setInputEventCallback(EventDelegateInput callback);
	
	void initializeJoysticks(ui8 count);
	void pollInput();

};

NS_END

#endif