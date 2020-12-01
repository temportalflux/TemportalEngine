#ifndef TE_DEPENDENCY_SDL_HPP
#define TE_DEPENDENCY_SDL_HPP

// Parents --------------------------------------------------------------------
#include "dependency/DependencyModule.hpp"

// ----------------------------------------------------------------------------
NS_DEPENDENCY

/**
* Module to initialize the SDL library as a dependency for the engine.
*/
class TEMPORTALENGINE_API SDL : public Module
{
public:

	SDL();

	/**
	* Initializes and sets up SDL as a depencency module.
	*/
	bool initialize() override;

	/**
	* Deconstructs and terminates SDL as a depencency module.
	*/
	void terminate() override;

};

NS_END
// ----------------------------------------------------------------------------

#endif