#include <iostream>
#include <GLFW/glfw3.h>
#include "Log.h"

using namespace std;

#define LogEngine "TemportalEngine"

void error_callback(int error, const char* description)
{
	logging::log(LogEngine, logging::ECategory::ERROR, description);
}

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GLFW_TRUE);
}

int main()
{
	logging::log(LogEngine, logging::ECategory::INFO, "Hello World!");

	glfwSetErrorCallback(error_callback);
	if (!glfwInit())
	{
		logging::log(LogEngine, logging::ECategory::ERROR, "Failed to initialize GLFW");
		return 1;
	}

	GLFWwindow* window = glfwCreateWindow(640, 480, "Temportal Engine", nullptr, nullptr);
	if (!window)
	{
		// Window or OpenGL context creation failed
		logging::log(LogEngine, logging::ECategory::ERROR, "Failed to create GLFW window");
		glfwTerminate();
		return 1;
	}

	glfwSetKeyCallback(window, key_callback);

	glfwMakeContextCurrent(window);
	//gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);
	glfwSwapInterval(1);

	while (!glfwWindowShouldClose(window))
	{
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}