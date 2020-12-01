#include "Module.hpp"
#include "Engine.hpp"

MODULE_INIT()
{
	auto LOG = DeclareLog("TestModule");
	LOG.log(LOG_INFO, "Test module has initialized");
}
