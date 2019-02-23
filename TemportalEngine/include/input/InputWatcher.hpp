#ifndef INPUT_WATCHER_HPP
#define INPUT_WATCHER_HPP

#include "Namespace.h"
#include "types/integer.h"
#include "input/Event.hpp"

NS_INPUT

class TEMPORTALENGINE_API InputWatcher
{
public:
	typedef void(*DelegateKeyCallback)(Event const &evt);

private:

	static ui8 const MAX_JOYSTICK_COUNT = 8;

	DelegateKeyCallback mpDelegateCallback;

private:

	ui8 mJoystickCount;
	void* mpJoystickHandles[MAX_JOYSTICK_COUNT];

	void executeInputCallback(Event const &evt);

public:
	InputWatcher();
	InputWatcher(DelegateKeyCallback callback);
	~InputWatcher();

	void setCallback(DelegateKeyCallback callback);
	void initializeJoysticks(ui8 count);
	void pollInput();

};

NS_END

#endif