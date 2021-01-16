#include "Module.hpp"

#include "Engine.hpp"

#ifdef _WIN32
#pragma comment(lib, "rpcrt4.lib")  // UuidCreate - Minimum supported OS Win 2000
#include "Windows.h"
#endif

static logging::Logger MODULE_LOADER_LOG = DeclareLog("ModuleLoader", LOG_INFO);

// Loading DLLs at runtime on windows:
// https://docs.microsoft.com/en-us/windows/win32/dlls/using-run-time-dynamic-linking
// and on unix:
// https://tldp.org/HOWTO/html_single/C++-dlopen/

void* loadDll(std::filesystem::path const& path)
{
	auto strAbsPath = std::filesystem::absolute(path).string();
#ifdef _WIN32
	LPCSTR absPath = strAbsPath.c_str();
	return (void*)LoadLibrary(absPath);
#else
	return nullptr; // TODO: dlopen
#endif
}

bool tryInitModule(void* handle)
{
#ifdef _WIN32
	TInitModule initModule = (TInitModule)GetProcAddress((HMODULE)handle, "initModule");
#else
	TInitModule initModule = nullptr; // TODO: unix
	return false;
#endif
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
#ifdef _WIN32
	FreeLibrary((HMODULE)handle);
#else
	// TODO: dlclose
#endif
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
