#pragma once

#include "TemportalEnginePCH.hpp"

#include "logging/Logger.hpp"

NS_NETWORK

enum class EType : ui8
{
	eClient = 0,
	eServer = 1,
};
enum class EClientStatus : ui8
{
	eDisconnected = 0,
	eConnected = 1,
};

logging::Logger& logger();
std::optional<std::string> init();
void uninit();

NS_END
