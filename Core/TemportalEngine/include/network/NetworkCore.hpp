#pragma once

#include "TemportalEnginePCH.hpp"

#include "logging/Logger.hpp"

NS_NETWORK

enum class EType : ui8
{
	eClient = 0b01,
	eServer = 0b10,
};
enum class EClientStatus : ui8
{
	eDisconnected = 0,
	eAuthenticating = 1,
	eConnected = 2,
};

logging::Logger& logger();
std::optional<std::string> init();
void uninit();

NS_END
