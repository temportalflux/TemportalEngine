#include "Module.hpp"

#include "Engine.hpp"

#ifdef _WIN32
#pragma comment(lib, "rpcrt4.lib")  // UuidCreate - Minimum supported OS Win 2000
#include "Windows.h"
#endif

static logging::Logger MODULE_LOADER_LOG = DeclareLog("ModuleLoader");

// https://docs.microsoft.com/en-us/windows/win32/dlls/using-run-time-dynamic-linking
// https://tldp.org/HOWTO/html_single/C++-dlopen/
void* loadDll(std::filesystem::path const& path)
{
	auto strAbsPath = std::filesystem::absolute(path).string();
	LPCSTR absPath = strAbsPath.c_str();
	return (void*)LoadLibrary(absPath);
}

bool tryInitModule(void* handle)
{
	auto initModule = (TInitModule)GetProcAddress((HMODULE)handle, "initModule");
	if (initModule == nullptr)
	{
		MODULE_LOADER_LOG.log(LOG_ERR, "Failed to find initModule entrance for dll module.");
		return false;
	}
	(initModule)();
	return true;
}

void freeDLL(void* handle)
{
	FreeLibrary((HMODULE)handle);
}

bool module_ext::loadModule(std::filesystem::path const& path)
{
	auto handle = loadDll(path);
	if (handle == nullptr)
	{
		MODULE_LOADER_LOG.log(LOG_ERR, "Failed to load dll module at %s", path.string().c_str());
		return false;
	}
	auto bInitialized = tryInitModule(handle);
	freeDLL(handle);
	return bInitialized;
}
