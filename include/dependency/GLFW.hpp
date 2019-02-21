#ifndef DEPENDENCY_GLFW_H
#define DEPENDENCY_GLFW_H

#include "Dependency.hpp"

class GLFW : public Dependency
{
public:

	GLFW();
	bool initialize() override;
	void terminate() override;
};

#endif  // DEPENDENCY_GLFW_H
