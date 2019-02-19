#ifndef DEPENDENCY_GLFW_H
#define DEPENDENCY_GLFW_H

#include "Dependency.h"

#define LogGlfw "GLFW"

//#define LogGlfwError(message, ...)
//	logging::log(LogGlfw, logging::ECategory::ERROR, message, __VA_ARGS__);

class GLFW : public Dependency
{
public:

	GLFW();
	bool initialize() override;
	void terminate() override;
};

#endif  // DEPENDENCY_GLFW_H
