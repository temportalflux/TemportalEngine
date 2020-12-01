#include "dependency/DependencyModule.hpp"

using namespace dependency;

// Module ---------------------------------------------------------------------

Module::Module()
{
	markAsInitialized(false);
}

Module::~Module()
{
	markAsInitialized(false);
}

bool Module::markAsInitialized(bool const initialized)
{
	mIsInitialized = initialized;
	return mIsInitialized;
}

bool const Module::isInitialized() const
{
	return mIsInitialized;
}

void Module::terminate()
{
	/* NO-OP */
}

// ModuleNull -----------------------------------------------------------------

bool ModuleNull::initialize()
{
	/* NO-OP */
	return false;
}
