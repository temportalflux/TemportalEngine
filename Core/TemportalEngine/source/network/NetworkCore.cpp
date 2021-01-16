#include "network/NetworkCore.hpp"

#include "Engine.hpp"
#include "logging/Logger.hpp"

#include <steam/steamnetworkingsockets.h>
#include <steam/isteamnetworkingutils.h>

using namespace network;

logging::Logger NETWORK_LOG = DeclareLog("Network");

std::vector<EType> utility::EnumWrapper<EType>::ALL = {
	EType::eClient,
	EType::eServer,
};
std::string utility::EnumWrapper<EType>::to_string() const
{
	switch (value())
	{
	case EType::eClient: return "client";
	case EType::eServer: return "server";
	default: return "invalid";
	}
}
std::string utility::EnumWrapper<EType>::to_display_string() const { return to_string(); }

logging::Logger& network::logger()
{
	return NETWORK_LOG;
}

void OutputDebugNetworkLog(ESteamNetworkingSocketsDebugOutputType type, char const* msg)
{
	logging::ELogLevel category;
	bool bIsFatal = false;
	switch (type)
	{
		case k_ESteamNetworkingSocketsDebugOutputType_Bug:
			category = LOG_ERR;
			bIsFatal = true;
			break;
		case k_ESteamNetworkingSocketsDebugOutputType_Error:
			category = LOG_ERR;
			break;
		case k_ESteamNetworkingSocketsDebugOutputType_Warning:
			category = LOG_WARN;
			break;
		case k_ESteamNetworkingSocketsDebugOutputType_Important:
			category = LOG_INFO;
			break;
		default: return;
	}
	if (!bIsFatal) NETWORK_LOG.log(category, msg);
	else NETWORK_LOG.log(category, "FATAL: %s", msg);
}

std::optional<std::string> network::init()
{
	NETWORK_LOG.log(LOG_INFO, "Initializing interface");
	SteamNetworkingErrMsg errorMessage;
	if (!GameNetworkingSockets_Init(nullptr, errorMessage))
	{
		return std::string(errorMessage);
	}
	SteamNetworkingUtils()->SetDebugOutputFunction(
		k_ESteamNetworkingSocketsDebugOutputType_Msg, OutputDebugNetworkLog
	);
	return std::nullopt;
}

void network::uninit()
{
	NETWORK_LOG.log(LOG_INFO, "Uninitializing interface");
	GameNetworkingSockets_Kill();
}
