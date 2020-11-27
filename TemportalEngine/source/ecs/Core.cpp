#include "ecs/Core.hpp"

using namespace ecs;

Core::Core()
	: mComponentManager(this)
{
}

Core::~Core()
{
}

Core& Core::setLog(logging::Logger log)
{
	this->mLog = log;
	return *this;
}

EntityManager& Core::entities() { return this->mEntityManager; }

ComponentManager& Core::components() { return this->mComponentManager; }
