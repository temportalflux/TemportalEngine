#include "input/InputCore.hpp"

#include <SDL.h>

void input::startTextInput()
{
	SDL_StartTextInput();
}

void input::stopTextInput()
{
	SDL_StopTextInput();
}

bool input::isTextInputActive()
{
	return SDL_IsTextInputActive();
}
