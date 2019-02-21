#include "dependency/GLFW.hpp"
#include <GLFW/glfw3.h>
#include "logging/Logger.hpp"
#include "Engine.hpp"

#define LogGlfw(cate, ...) logging::Logger("Glfw", &engine::Engine::LOG_SYSTEM).log(cate, __VA_ARGS__);

using namespace logging;

void glfwErrorCallback(int error, const char* description)
{
	LogGlfw(logging::ECategory::ERROR, "%i - %s", error, description);
}

GLFW::GLFW()
	: Dependency()
{

}

bool GLFW::initialize()
{
	LogGlfw(ECategory::INFO, "Initializing");

	LogGlfw(ECategory::INFO,
				 "Compiled against %i.%i.%i",
				 GLFW_VERSION_MAJOR,
				 GLFW_VERSION_MINOR,
				 GLFW_VERSION_REVISION);

	glfwSetErrorCallback(&glfwErrorCallback);
	if (!glfwInit())
	{
		LogGlfw(ECategory::ERROR, "Failed to initialize");
		return false;
	}

	int major, minor, revision;
	glfwGetVersion(&major, &minor, &revision);
	LogGlfw(ECategory::INFO,
				 "Running against %i.%i.%i", major, minor, revision);

	this->setInitialized(true);
	return true;
}

void GLFW::terminate()
{
	LogGlfw(ECategory::INFO, "Terminating");
	glfwTerminate();
	this->setInitialized(false);
}

