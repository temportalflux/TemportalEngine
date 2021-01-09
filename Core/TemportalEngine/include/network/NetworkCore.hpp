#pragma once

#include "TemportalEnginePCH.hpp"

#include "logging/Logger.hpp"

NS_NETWORK

logging::Logger& logger();
std::optional<std::string> init();
void uninit();

NS_END
