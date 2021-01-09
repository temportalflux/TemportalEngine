#include "network/NetworkInterface.hpp"

#include "game/GameInstance.hpp"
#include "network/NetworkCore.hpp"
#include "utility/Casting.hpp"

#include <steam/steamnetworkingsockets.h>
#include <steam/isteamnetworkingutils.h>

using namespace network;

SteamNetworkingConfigValue_t makeConfigCallback(ESteamNetworkingConfigValue key, network::Interface *interface, void (network::Interface::*f)(void*))
{
	std::function<void(void*)> callback = std::bind(f, interface, std::placeholders::_1);
	SteamNetworkingConfigValue_t option = {};
	option.SetPtr(k_ESteamNetworkingConfig_Callback_ConnectionStatusChanged, callback.target<void(void*)>());
	return option;
}

Interface::Interface() : mpInternal(nullptr), mType(EType::eInvalid)
{
}

Interface& Interface::setType(EType type)
{
	this->mType = type;
	return *this;
}

Interface& Interface::setAddress(Address const& address)
{
	this->mAddress = address;
	return *this;
}

void connectionCallback(SteamNetConnectionStatusChangedCallback_t *pInfo)
{
	auto& netInterface = game::Game::Get()->networkInterface();
	switch (netInterface.type())
	{
	case network::Interface::EType::eServer:
		netInterface.onServerConnectionStatusChanged((void*)pInfo);
		break;
	case network::Interface::EType::eClient:
		netInterface.onClientConnectionStatusChanged((void*)pInfo);
		break;
	default: break;
	}
}

void Interface::start()
{
	if (this->mType == EType::eInvalid) return;

	this->mpInternal = SteamNetworkingSockets();
	auto* pInterface = as<ISteamNetworkingSockets>(this->mpInternal);

	auto address = *as<SteamNetworkingIPAddr>(this->mAddress.get());
	auto options = std::vector<SteamNetworkingConfigValue_t>();

	SteamNetworkingConfigValue_t option = {};
	option.SetPtr(k_ESteamNetworkingConfig_Callback_ConnectionStatusChanged, &connectionCallback);
	options.push_back(option);

	if (this->mType == EType::eServer)
	{
		network::logger().log(LOG_INFO, "Listening at IP %s", this->mAddress.toString(true).c_str());
		this->mConnection = pInterface->CreateListenSocketIP(address, (ui32)options.size(), options.data());
		if (this->mConnection == k_HSteamListenSocket_Invalid)
		{
			network::logger().log(LOG_ERR, "Failed to create listen socket on port %d", this->mAddress.port());
			return;
		}
		
		this->mServerPollGroup = pInterface->CreatePollGroup();
		if (this->mServerPollGroup == k_HSteamNetPollGroup_Invalid)
		{
			network::logger().log(LOG_ERR, "Failed to create listen socket on port %d", this->mAddress.port());
			return;
		}
	}
	else if (this->mType == EType::eClient)
	{
		network::logger().log(LOG_INFO, "Connecting to server at %s", this->mAddress.toString(true).c_str());
		this->mConnection = pInterface->ConnectByIPAddress(address, (ui32)options.size(), options.data());
		if (this->mConnection == k_HSteamNetConnection_Invalid)
		{
			network::logger().log(LOG_ERR, "Failed to create connection to %s", this->mAddress.toString(true).c_str());
			return;
		}
	}

}

void Interface::stop()
{
	if (this->mpInternal == nullptr) return;

	auto* pInterface = as<ISteamNetworkingSockets>(this->mpInternal);
	if (this->mType == EType::eServer)
	{
		network::logger().log(LOG_INFO, "Server is shutting down.");

		for (auto const& clientId : this->mClientIds)
		{
			pInterface->CloseConnection(clientId, 0, "Server Shutdown", true);
		}
		this->mClientIds.clear();

		if (this->mConnection != k_HSteamListenSocket_Invalid)
		{
			pInterface->CloseListenSocket(this->mConnection);
			this->mConnection = k_HSteamListenSocket_Invalid;
		}

		if (this->mServerPollGroup != k_HSteamNetPollGroup_Invalid)
		{
			pInterface->DestroyPollGroup(this->mServerPollGroup);
			this->mServerPollGroup = k_HSteamNetPollGroup_Invalid;
		}
	}
	else if (this->mType == EType::eClient)
	{
		if (this->mConnection != k_HSteamNetConnection_Invalid)
		{
			pInterface->CloseConnection(this->mConnection, 0, nullptr, false);
			this->mConnection = k_HSteamNetConnection_Invalid;
		}
	}

	this->mpInternal = nullptr;
}

bool Interface::hasConnection() const
{
	switch (this->mType)
	{
	case EType::eClient: return this->mConnection != k_HSteamNetConnection_Invalid;
	case EType::eServer: return this->mConnection != k_HSteamListenSocket_Invalid;
	default: return false;
	}
}

void Interface::update(f32 deltaTime)
{
	if (!this->hasConnection()) return;
	assert(this->mpInternal != nullptr);
	this->pollIncomingMessages();
	as<ISteamNetworkingSockets>(this->mpInternal)->RunCallbacks();
}

void Interface::pollIncomingMessages()
{
	auto* pInterface = as<ISteamNetworkingSockets>(this->mpInternal);
	ISteamNetworkingMessage *pIncomingMsg = nullptr;
	i32 numMsgs = -1;

	if (this->mType == EType::eServer)
	{
		numMsgs = pInterface->ReceiveMessagesOnPollGroup(this->mServerPollGroup, &pIncomingMsg, 1);
	}
	else if (this->mType == EType::eClient)
	{
		numMsgs = pInterface->ReceiveMessagesOnConnection(this->mConnection, &pIncomingMsg, 1);
	}
	else
	{
		assert(false);
	}

	if (numMsgs == 0) return;
	else if (numMsgs < 0)
	{
		network::logger().log(LOG_ERR, "Failed to poll for messages.");
		return;
	}
	assert(numMsgs == 1 && pIncomingMsg);
	// TODO: Add message to queue
	pIncomingMsg->Release();
}

void Interface::onServerConnectionStatusChanged(void* pInfo)
{
	assert(this->mType == Interface::EType::eServer);
	auto* pInterface = as<ISteamNetworkingSockets>(this->mpInternal);
	auto data = (SteamNetConnectionStatusChangedCallback_t*)pInfo;
	switch (data->m_info.m_eState)
	{
	// Happens when a server destroys a connection with a client
	case k_ESteamNetworkingConnectionState_None: break;

	case k_ESteamNetworkingConnectionState_Connecting:
	{
		if (pInterface->AcceptConnection(data->m_hConn) != k_EResultOK)
		{
			// Ensure the client is cleaned up if the connection accept failed
			pInterface->CloseConnection(data->m_hConn, 0, nullptr, false);
			network::logger().log(LOG_INFO, "Failed to accept connection, it may be already closed.");
			return;
		}

		if (!pInterface->SetConnectionPollGroup(data->m_hConn, this->mServerPollGroup))
		{
			pInterface->CloseConnection(data->m_hConn, 0, nullptr, false);
			network::logger().log(LOG_ERR, "Failed to assign a poll group.");
			return;
		}

		this->mClientIds.insert(data->m_hConn);

		network::logger().log(
			LOG_INFO, "Accepted connection request from %s",
			data->m_info.m_szConnectionDescription
		);
		break;
	}

	// NO-OP: happens immediately after we, the server, accept the connection
	case k_ESteamNetworkingConnectionState_Connected: break;

	case k_ESteamNetworkingConnectionState_ClosedByPeer:
	case k_ESteamNetworkingConnectionState_ProblemDetectedLocally:
	{
		// If the connection was not previously connected, then the user dropped before it could be processed
		if (data->m_eOldState == k_ESteamNetworkingConnectionState_Connected)
		{
			const char *disconnectionCause;
			if (data->m_info.m_eState == k_ESteamNetworkingConnectionState_ProblemDetectedLocally)
			{
				disconnectionCause = "problem detected locally";
			}
			else
			{
				disconnectionCause = "closed by peer";
			}

			network::logger().log(
				LOG_INFO, "Connection %s %s, reason %d: %s",
				data->m_info.m_szConnectionDescription, disconnectionCause,
				data->m_info.m_eEndReason, data->m_info.m_szEndDebug
			);

			this->mClientIds.erase(this->mClientIds.find(data->m_hConn));
		}
		// Actually destroy the connection data in the API
		pInterface->CloseConnection(data->m_hConn, 0, nullptr, false);
		break;
	}

	default: break;
	}
}

void Interface::onClientConnectionStatusChanged(void* pInfo)
{
	assert(this->mType == Interface::EType::eClient);
	auto* pInterface = as<ISteamNetworkingSockets>(this->mpInternal);
	auto data = (SteamNetConnectionStatusChangedCallback_t*)pInfo;
	assert(data->m_hConn == this->mConnection || this->mConnection == k_HSteamNetConnection_Invalid);
	switch (data->m_info.m_eState)
	{
	// NO-OP: the client destroyed its connection
	case k_ESteamNetworkingConnectionState_None: break;

	// NO-OP: the client has started connecting
	case k_ESteamNetworkingConnectionState_Connecting: break;

	case k_ESteamNetworkingConnectionState_Connected:
	{
		network::logger().log(LOG_INFO, "Successfully connected to server.");
		break;
	}

	case k_ESteamNetworkingConnectionState_ClosedByPeer:
	case k_ESteamNetworkingConnectionState_ProblemDetectedLocally:
	{
		if (data->m_eOldState == k_ESteamNetworkingConnectionState_Connecting)
		{
			// soft-todo: distinguish between a timeout, a rejected connection, or some other transport problem.
			network::logger().log(LOG_INFO, "Disconnected from server: %s", data->m_info.m_szEndDebug);
		}
		else if (data->m_info.m_eState == k_ESteamNetworkingConnectionState_ProblemDetectedLocally)
		{
			network::logger().log(LOG_INFO, "Lost server connection: %s", data->m_info.m_szEndDebug);
		}
		else
		{
			// soft-todo: check the reason code for a normal disconnection
			network::logger().log(LOG_INFO, "Disconnected from server: %s", data->m_info.m_szEndDebug);
		}

		pInterface->CloseConnection(data->m_hConn, 0, nullptr, false);
		this->mConnection = k_HSteamNetConnection_Invalid;
		break;
	}

	default: break;
	}
}
