#ifndef DEPENDENCY_GLFW_H
#define DEPENDENCY_GLFW_H

#include "Api.h"

#include "Dependency.hpp"

class TEMPORTALENGINE_API SDL : public Dependency
{
public:

	SDL();
	bool initialize() override;
	void terminate() override;
};

#endif  // DEPENDENCY_GLFW_H
