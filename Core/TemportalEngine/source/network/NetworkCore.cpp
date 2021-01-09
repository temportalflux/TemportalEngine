#include "network/NetworkCore.hpp"

#include "Engine.hpp"
#include "logging/Logger.hpp"

#include <steam/steamnetworkingsockets.h>
#include <steam/isteamnetworkingutils.h>

logging::Logger NETWORK_LOG = DeclareLog("Network");

void OutputDebugNetworkLog(ESteamNetworkingSocketsDebugOutputType type, char const* msg)
{
	logging::ECategory category = logging::ECategory::INVALID;
	bool bIsFatal = false;
	switch (type)
	{
		case k_ESteamNetworkingSocketsDebugOutputType_Bug:
			category = logging::ECategory::LOGERROR;
			bIsFatal = true;
			break;
		case k_ESteamNetworkingSocketsDebugOutputType_Error:
			category = logging::ECategory::LOGERROR;
			break;
		case k_ESteamNetworkingSocketsDebugOutputType_Warning:
			category = logging::ECategory::LOGWARN;
			break;
		case k_ESteamNetworkingSocketsDebugOutputType_Important:
			category = logging::ECategory::LOGINFO;
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
