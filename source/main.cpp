#include <iostream>
#include <GLFW/glfw3.h>
#include "Log.h"

using namespace std;

int main()
{
	logging::log("TemportalEngine", logging::ECategory::INFO, "Hello World!");

	if (!glfwInit())
	{
		logging::log("TemportalEngine", logging::ECategory::INFO, "Failed to initialize GLFW");
	}
	glfwTerminate();

	return 0;
}