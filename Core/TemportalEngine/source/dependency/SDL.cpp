#include "dependency/SDL.hpp"

// Libraries ------------------------------------------------------------------
#include <SDL.h>

// Engine ---------------------------------------------------------------------
#include "Engine.hpp"
#include "logging/Logger.hpp"

// Logging --------------------------------------------------------------------
#define LogSDL(cate, ...) logging::Logger("SDL", LOG_INFO, &engine::Engine::LOG_SYSTEM).log(cate, __VA_ARGS__);

// Namespace ------------------------------------------------------------------
using namespace logging;
using namespace dependency;

// Statics --------------------------------------------------------------------
void glfwErrorCallback(int error, const char* description)
{
	LogSDL(LOG_ERR, "%i - %s", error, description);
}

// SDL ------------------------------------------------------------------------

SDL::SDL() : Module()
{

}

bool SDL::initialize()
{
	LogSDL(LOG_INFO, "Initializing");

	SDL_version version;

	SDL_VERSION(&version);

	LogSDL(LOG_INFO,
		"Compiled against %i.%i.%i",
		version.major, version.minor, version.patch
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
		LogSDL(LOG_ERR, "Failed to initialize SDL::VIDEO, %s", SDL_GetError());
		return false;
	}
	if (SDL_Init(SDL_INIT_AUDIO) != 0)
	{
		LogSDL(LOG_ERR, "Failed to initialize SDL::AUDIO, %s", SDL_GetError());
		return false;
	}
	if (SDL_Init(SDL_INIT_JOYSTICK) != 0)
	{
		LogSDL(LOG_ERR, "Failed to initialize SDL::JOYSTICK, %s", SDL_GetError());
		return false;
	}
	if (SDL_Init(SDL_INIT_HAPTIC) != 0)
	{
		LogSDL(LOG_ERR, "Failed to initialize SDL::HAPTIC, %s", SDL_GetError());
		return false;
	}
	if (SDL_Init(SDL_INIT_GAMECONTROLLER) != 0)
	{
		LogSDL(LOG_ERR, "Failed to initialize SDL::GAMECONTROLLER, %s", SDL_GetError());
		return false;
	}

	SDL_GetVersion(&version);
	LogSDL(LOG_INFO,
		"Running against %i.%i.%i",
		version.major, version.minor, version.patch
	);

	return this->markAsInitialized(true);
}

void SDL::terminate()
{
	LogSDL(LOG_INFO, "Terminating");
	SDL_Quit();
	this->markAsInitialized(false);
}

