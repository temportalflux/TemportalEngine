#ifndef INPUT_EVENT_HPP
#define INPUT_EVENT_HPP

#include "Namespace.h"
#include "Key.hpp"

NS_INPUT

struct Event
{
	i32 key;
	i32 scanCode;
	i32 action;
	i32 mods;
};

NS_END

#endif // INPUT_EVENT_HPP
