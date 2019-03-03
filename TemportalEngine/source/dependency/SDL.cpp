#include "dependency/SDL.hpp"
#include <SDL.h>
#include "logging/Logger.hpp"
#include "Engine.hpp"

#define LogSDL(cate, ...) logging::Logger("SDL", &engine::Engine::LOG_SYSTEM).log(cate, __VA_ARGS__);

using namespace logging;

void glfwErrorCallback(int error, const char* description)
{
	LogSDL(logging::ECategory::LOGERROR, "%i - %s", error, description);
}

SDL::SDL() : Dependency()
{

}

bool SDL::initialize()
{
	LogSDL(ECategory::LOGINFO, "Initializing");

	SDL_version version;

	SDL_VERSION(&version);

	LogSDL(ECategory::LOGINFO,
		"Compiled against %i.%i.%i", version.major, version.minor, version.patch
	);

	/*
	SDL_INIT_TIMER
	SDL_INIT_AUDIO
	SDL_INIT_VIDEO
	SDL_INIT_JOYSTICK
	SDL_INIT_HAPTIC
	SDL_INIT_GAMECONTROLLER
	SDL_INIT_EVENTS
	SDL_INIT_SENSOR
	SDL_INIT_NOPARACHUTE
	*/
	if (SDL_Init(SDL_INIT_VIDEO) != 0)
	{
		LogSDL(ECategory::LOGERROR, "Failed to initialize SDL::VIDEO");
		return false;
	}
	if (SDL_Init(SDL_INIT_AUDIO) != 0)
	{
		LogSDL(ECategory::LOGERROR, "Failed to initialize SDL::AUDIO");
		return false;
	}
	if (SDL_Init(SDL_INIT_JOYSTICK) != 0)
	{
		LogSDL(ECategory::LOGERROR, "Failed to initialize SDL::JOYSTICK");
		return false;
	}
	if (SDL_Init(SDL_INIT_HAPTIC) != 0)
	{
		LogSDL(ECategory::LOGERROR, "Failed to initialize SDL::HAPTIC");
		return false;
	}
	if (SDL_Init(SDL_INIT_GAMECONTROLLER) != 0)
	{
		LogSDL(ECategory::LOGERROR, "Failed to initialize SDL::GAMECONTROLLER");
		return false;
	}

	SDL_GetVersion(&version);
	LogSDL(ECategory::LOGINFO,
		"Running against %i.%i.%i", version.major, version.minor, version.patch);

	this->setInitialized(true);
	return true;
}

void SDL::terminate()
{
	LogSDL(ECategory::LOGINFO, "Terminating");
	SDL_Quit();
	this->setInitialized(false);
}

