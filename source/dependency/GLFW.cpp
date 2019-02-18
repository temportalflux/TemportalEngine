#include "dependency/GLFW.h"
#include <GLFW/glfw3.h>
#include <dependency/GLFW.h>
#include "Log.h"

using namespace logging;

void glfwErrorCallback(int error, const char* description)
{
	LogGlfwError("%i - %s", error, description);
}

GLFW::GLFW()
	: Dependency()
{

}

bool GLFW::initialize()
{
	log(LogGlfw, ECategory::INFO, "Initializing");

	log(LogGlfw, ECategory::INFO,
				 "Compiled against %i.%i.%i",
				 GLFW_VERSION_MAJOR,
				 GLFW_VERSION_MINOR,
				 GLFW_VERSION_REVISION);

	glfwSetErrorCallback(&glfwErrorCallback);
	if (!glfwInit())
	{
		LogGlfwError("Failed to initialize");
		return false;
	}

	int major, minor, revision;
	glfwGetVersion(&major, &minor, &revision);
	log(LogGlfw, ECategory::INFO,
				 "Running against %i.%i.%i", major, minor, revision);

	this->setInitialized(true);
	return true;
}

void GLFW::terminate()
{
	log(LogGlfw, ECategory::INFO, "Terminating");
	glfwTerminate();
	this->setInitialized(false);
}

