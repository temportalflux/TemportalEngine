#include <dependency/Dependency.h>
#include <Window.h>
#include "Log.h"
#include "dependency/GLFW.h"
#include <GLFW/glfw3.h>

using namespace std;

#define LogEngine "TemportalEngine"

static void key_callback(Window* window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
	{
		window->markShouldClose();
	}
}

int main()
{
	logging::log(LogEngine, logging::ECategory::INFO, "Hello World!");

	GLFW pDepGlfw[1];
	*pDepGlfw = GLFW();

	if (!pDepGlfw->initialize()) return 1;

	Window pWindow[1];
	*pWindow = Window(640, 480, "Temportal Engine");
	if (!pWindow->isValid())
	{
		pDepGlfw->terminate();
		return 1;
	}

	pWindow->setKeyCallback(key_callback);

	pWindow->initializeRenderContext(1);;

	while (!pWindow->isClosePending()) pWindow->update();

	pWindow->destroy();
	pDepGlfw->terminate();

	return 0;
}