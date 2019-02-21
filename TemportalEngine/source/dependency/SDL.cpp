#include "dependency/SDL.hpp"
#include <SDL.h>
#include "logging/Logger.hpp"
#include "Engine.hpp"

#define LogSDL(cate, ...) logging::Logger("SDL", &engine::Engine::LOG_SYSTEM).log(cate, __VA_ARGS__);

using namespace logging;

void glfwErrorCallback(int error, const char* description)
{
	LogSDL(logging::ECategory::ERROR, "%i - %s", error, description);
}

SDL::SDL() : Dependency()
{

}

bool SDL::initialize()
{
	LogSDL(ECategory::INFO, "Initializing");

	SDL_version version;

	SDL_VERSION(&version);

	LogSDL(ECategory::INFO,
		"Compiled against %i.%i.%i", version.major, version.minor, version.patch
	);

	if (SDL_Init(SDL_INIT_VIDEO) != 0)
	{
		LogSDL(ECategory::ERROR, "Failed to initialize");
		return false;
	}

	SDL_GetVersion(&version);
	LogSDL(ECategory::INFO,
		"Running against %i.%i.%i", version.major, version.minor, version.patch);

	this->setInitialized(true);
	return true;
}

void SDL::terminate()
{
	LogSDL(ECategory::INFO, "Terminating");
	SDL_Quit();
	this->setInitialized(false);
}

