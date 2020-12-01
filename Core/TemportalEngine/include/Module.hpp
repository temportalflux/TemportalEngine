#pragma once

#include "TemportalEnginePCH.hpp"

#define NS_MODULE namespace module_ext {
#define MODULE_INIT extern "C" __declspec(dllexport) void initModule
typedef void(__cdecl  *TInitModule)();

NS_MODULE
bool loadModule(std::filesystem::path const& path);
NS_END
