#include "network/NetworkInterface.hpp"

#include "Engine.hpp"
#include "network/NetworkCore.hpp"
#include "network/NetworkPacket.hpp"
#include "network/packet/NetworkPacketClientStatus.hpp"
#include "network/packet/NetworkPacketECSReplicate.hpp"
#include "utility/Casting.hpp"

#include <steam/steamnetworkingsockets.h>
#include <steam/isteamnetworkingutils.h>

using namespace network;

#define PACKET_DATA_LOG_LEVEL LOG_VERBOSE

SteamNetworkingConfigValue_t makeConfigCallback(ESteamNetworkingConfigValue key, network::Interface *pInterface, void (network::Interface::*f)(void*))
{
	std::function<void(void*)> callback = std::bind(f, pInterface, std::placeholders::_1);
	SteamNetworkingConfigValue_t option = {};
	option.SetPtr(k_ESteamNetworkingConfig_Callback_ConnectionStatusChanged, callback.target<void(void*)>());
	return option;
}

Interface::Interface() : mpInternal(nullptr), mType(), mConnection(0)
{
	this->packetTypes()
		.addType<network::packet::ClientStatus>()
		.addType<network::packet::ECSReplicate>()
		;
}

Interface& Interface::setType(utility::Flags<network::EType> type)
{
	this->mType = type;
	return *this;
}

utility::Flags<network::EType> const& Interface::type() const { return this->mType; }

Interface& Interface::setAddress(Address const& address)
{
	this->mAddress = address;
	return *this;
}

void connectionCallback(SteamNetConnectionStatusChangedCallback_t *pInfo)
{
	auto& netInterface = engine::Engine::Get()->networkInterface();
	if (netInterface.type().includes(EType::eServer))
	{
		netInterface.onServerConnectionStatusChanged((void*)pInfo);
	}
	else if (netInterface.type() == EType::eClient)
	{
		netInterface.onClientConnectionStatusChanged((void*)pInfo);
	}
}

void Interface::start()
{
	if (!this->mType.hasValue()) return;

	this->mpInternal = SteamNetworkingSockets();
	auto* pInterface = as<ISteamNetworkingSockets>(this->mpInternal);

	auto address = *as<SteamNetworkingIPAddr>(this->mAddress.get());
	auto options = std::vector<SteamNetworkingConfigValue_t>();

	SteamNetworkingConfigValue_t option = {};
	option.SetPtr(k_ESteamNetworkingConfig_Callback_ConnectionStatusChanged, &connectionCallback);
	options.push_back(option);

	if (this->mType.includes(EType::eServer))
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

		this->onNetworkStarted.broadcast(this);

		if (this->type().includes(EType::eClient))
		{
			auto const netId = this->nextNetworkId();
			network::logger().log(
				LOG_VERBOSE, "Assigning same-app client (server connection %u) network-id %u",
				this->mConnection, netId
			);
			this->addClient(this->mConnection, netId);
			this->onConnectionEstablished.execute(this, this->mConnection, netId);
		}
	}
	else if (this->mType == EType::eClient)
	{
		network::logger().log(LOG_INFO, "Connecting to server at %s", this->mAddress.toString(true).c_str());
		this->mConnection = pInterface->ConnectByIPAddress(address, (ui32)options.size(), options.data());
		network::logger().log(LOG_DEBUG, "Make connection %u", this->mConnection);
		if (this->mConnection == k_HSteamNetConnection_Invalid)
		{
			network::logger().log(LOG_ERR, "Failed to create connection to %s", this->mAddress.toString(true).c_str());
			return;
		}
		this->onNetworkStarted.broadcast(this);
	}
}

void Interface::stop()
{
	if (this->mpInternal == nullptr) return;

	auto* pInterface = as<ISteamNetworkingSockets>(this->mpInternal);
	if (this->mType.includes(EType::eServer))
	{
		network::logger().log(LOG_INFO, "Server is shutting down.");

		for (auto const& [connection, netId] : this->mClients)
		{
			pInterface->CloseConnection(connection, 0, "Server Shutdown", true);
		}
		this->mConnections.clear();
		this->mClients.clear();
		this->mNetIdToConnection.clear();

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
	else if (this->mType.includes(EType::eClient))
	{
		network::logger().log(LOG_INFO, "Client is shutting down.");
		if (this->mConnection != k_HSteamNetConnection_Invalid)
		{
			pInterface->CloseConnection(this->mConnection, 0, nullptr, false);
			this->mConnection = k_HSteamNetConnection_Invalid;
		}
	}

	this->mpInternal = nullptr;
	this->onNetworkStopped.broadcast(this);
}

bool Interface::hasConnection() const
{
	if (this->type().includes(EType::eServer)) return this->mConnection != k_HSteamListenSocket_Invalid;
	else return this->mConnection != k_HSteamNetConnection_Invalid;
}

void Interface::update(f32 deltaTime)
{
	if (this->mpInternal == nullptr) return;
	if (!this->hasConnection()) return;
	
	this->pollIncomingMessages();
	
	while (this->mReceivedPackets.size() > 0)
	{
		auto iter = this->mReceivedPackets.begin();
		auto const& received = *iter;
		if (received.debugLog)
		{
			network::logger().log(PACKET_DATA_LOG_LEVEL, received.debugLog->c_str());
		}
		received.packet->process(this);
		this->mReceivedPackets.erase(iter);
	}
	
	as<ISteamNetworkingSockets>(this->mpInternal)->RunCallbacks();
}

void Interface::pollIncomingMessages()
{
	auto* pInterface = as<ISteamNetworkingSockets>(this->mpInternal);
	ISteamNetworkingMessage *pIncomingMsg = nullptr;
	i32 numMsgs = -1;

	if (this->mType.includes(EType::eServer))
	{
		numMsgs = pInterface->ReceiveMessagesOnPollGroup(this->mServerPollGroup, &pIncomingMsg, 1);
	}
	else if (this->mType.includes(EType::eClient))
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
	
	RecievedPacket received = {};
	{
		auto pPacketTypeId = reinterpret_cast<ui32*>(pIncomingMsg->m_pData);
		received.packet = this->packetTypes().create(*pPacketTypeId);
		received.packet->setSourceConnection(pIncomingMsg->m_conn);

		auto buffer = network::Buffer(
			this->type().includes(EType::eServer) ? EType::eClient : EType::eServer,
			pIncomingMsg->m_pData, pIncomingMsg->m_cbSize, true
		);
		received.packet->read(buffer);

		if (network::logger().isLevelEnabled(PACKET_DATA_LOG_LEVEL))
		{
			std::stringstream debugLog;
			debugLog << "Received Packet:\n";
			received.packet->appendDebugLogHeader(debugLog);
			buffer.stringify(debugLog);
			received.debugLog = debugLog.str();
		}
	}
	pIncomingMsg->Release();

	this->mReceivedPackets.push_back(std::move(received));
}

void Interface::onServerConnectionStatusChanged(void* pInfo)
{
	assert(this->mType.includes(EType::eServer));
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

		ui32 const netId = this->nextNetworkId();
		this->addClient(data->m_hConn, netId);

		network::logger().log(
			LOG_INFO, "Accepted connection request from %s. Assigned network-id $i.",
			data->m_info.m_szConnectionDescription, netId
		);

		// Tell the client its own net id
		packet::ClientStatus::create()
			->setStatus(EClientStatus::eAuthenticating)
			.setIsSelf(true).setNetId(netId)
			.send(data->m_hConn);
		this->onConnectionEstablished.execute(this, data->m_hConn, netId);
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
			bool bClosedByPeer = data->m_info.m_eState != k_ESteamNetworkingConnectionState_ProblemDetectedLocally;
			if (!bClosedByPeer) disconnectionCause = "problem detected locally";
			else disconnectionCause = "closed by peer";
			network::logger().log(
				LOG_INFO, "Connection %s %s, reason %d: %s",
				data->m_info.m_szConnectionDescription, disconnectionCause,
				data->m_info.m_eEndReason, data->m_info.m_szEndDebug
			);

			if (bClosedByPeer)
				this->OnDedicatedClientDisconnected.execute(this, this->mClients.find(data->m_hConn)->second);
			this->closeConnection(data->m_hConn);
		}
		else
		{
			// Actually destroy the connection data in the API
			pInterface->CloseConnection(data->m_hConn, 0, nullptr, false);
		}
		break;
	}

	default: break;
	}
}

void Interface::onClientConnectionStatusChanged(void* pInfo)
{
	assert(this->mType == EType::eClient);
	auto* pInterface = as<ISteamNetworkingSockets>(this->mpInternal);
	auto data = (SteamNetConnectionStatusChangedCallback_t*)pInfo;
	this->mConnection = data->m_hConn;
	switch (data->m_info.m_eState)
	{
	// NO-OP: the client destroyed its connection
	case k_ESteamNetworkingConnectionState_None: break;

	// NO-OP: the client has started connecting
	case k_ESteamNetworkingConnectionState_Connecting: break;

	case k_ESteamNetworkingConnectionState_Connected:
	{
		network::logger().log(LOG_INFO, "Successfully connected to server.");
		this->onConnectionEstablished.execute(this, data->m_hConn, 0);
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
		this->OnDedicatedClientDisconnected.execute(this, 0);
		break;
	}

	default: break;
	}
}

void Interface::sendPackets(
	std::set<ui32> connections,
	std::vector<std::shared_ptr<packet::Packet>> const& packets,
	std::set<ui32> except
)
{
	OPTICK_EVENT();
	assert(this->mpInternal != nullptr);

	for (auto iter = connections.begin(); iter != connections.end();)
	{
		if (except.find(*iter) != except.end()) iter = connections.erase(iter);
		else ++iter;
	}
	if (connections.size() == 0) return;

	auto utils = SteamNetworkingUtils();
	auto messages = std::vector<SteamNetworkingMessage_t*>();

	auto const bIsLogEnabled = network::logger().isLevelEnabled(PACKET_DATA_LOG_LEVEL);
	std::stringstream debugLog;

	if (bIsLogEnabled)
	{
		debugLog << "Sending Packets:\n";
		for (auto const& connection : connections)
		{
			debugLog << "Connection(" << connection << ')';
			if (this->type().includes(EType::eServer))
			{
				debugLog << " / ";
				debugLog << "NetId(";
				auto iter = this->mClients.find(connection);
				if (iter != this->mClients.end()) debugLog << iter->second;
				else debugLog << "unknown";
				debugLog << ')';
			}
			debugLog << '\n';
		}
	}

	for (auto pPacket : packets)
	{
		auto buffer = network::Buffer(this->type(), nullptr, 0, true);
		pPacket->write(buffer);

		if (bIsLogEnabled)
		{
			pPacket->appendDebugLogHeader(debugLog);
			buffer.stringify(debugLog);
		}

		for (auto const& connection : connections)
		{
			auto message = utils->AllocateMessage((ui32)buffer.size());
			assert(message->m_cbSize == buffer.size());
			message->m_conn = connection;
			message->m_nFlags = ui32(pPacket->flags().data());
			{
				auto buffer = network::Buffer(this->type(), message->m_pData, message->m_cbSize, false);
				pPacket->write(buffer);
			}
			messages.push_back(message);
		}
	}

	if (bIsLogEnabled)
	{
		network::logger().log(PACKET_DATA_LOG_LEVEL, debugLog.str().c_str());
	}

	// All messages ptrs are owned by the interface after this call (no need to free them).
	// network::Packet objects are either discarded at then end of this stack,
	// or soon after because of `shared_ptr`.
	as<ISteamNetworkingSockets>(this->mpInternal)->SendMessages((ui32)messages.size(), messages.data(), nullptr);
}

std::set<ui32> Interface::connections() const
{
	return this->mConnections;
}

void Interface::addClient(ui32 connection, ui32 netId)
{
	this->mConnections.insert(connection);
	this->mClients.insert(std::make_pair(connection, netId));
	this->mNetIdToConnection.insert(std::make_pair(netId, connection));
}

std::set<ui32> Interface::connectedClientNetIds() const
{
	auto ids = std::set<ui32>();
	for (auto const&[connection, netId] : this->mClients)
	{
		ids.insert(netId);
	}
	return ids;
}

ui32 Interface::nextNetworkId()
{
	if (this->mUnusedNetIds.size() > 0)
	{
		auto id = *this->mUnusedNetIds.begin();
		this->mUnusedNetIds.erase(this->mUnusedNetIds.begin());
		return id;
	}
	return (ui32)this->mConnections.size();
}

ui32 Interface::getNetIdFor(ui32 connection) const
{
	assert(this->type().includes(EType::eServer));
	auto iter = this->mClients.find(connection);
	assert(iter != this->mClients.end());
	return iter->second;
}

ui32 Interface::getConnectionFor(ui32 netId) const
{
	assert(this->type().includes(EType::eServer));
	auto iter = this->mNetIdToConnection.find(netId);
	assert(iter != this->mNetIdToConnection.end());
	return iter->second;
}

// Does not get executed for the net id of a client-on-top-of-server
void Interface::markClientAuthenticated(ui32 netId)
{
	auto connectionId = this->getConnectionFor(netId);

	packet::ClientStatus::create()
		->setStatus(EClientStatus::eConnected)
		.setIsSelf(true).setNetId(netId)
		.send(connectionId);

	// Tell all other clients that a client has joined
	packet::ClientStatus::create()
		->setStatus(EClientStatus::eConnected)
		.setIsSelf(false).setNetId(netId)
		.broadcast({ connectionId });
	if (this->type().includes(EType::eClient))
	{
		this->onClientPeerStatusChanged.execute(this, netId, EClientStatus::eConnected);
	}

	// Tell the client of the net ids of other already joined clients
	for (auto const&[otherConnectionId, otherNetId] : this->mClients)
	{
		if (otherNetId == netId) continue;
		packet::ClientStatus::create()
			->setStatus(EClientStatus::eConnected)
			.setIsSelf(false).setNetId(otherNetId)
			.send(connectionId);
	}

	this->OnDedicatedClientAuthenticated.execute(this, netId);
}

ui32 Interface::closeConnection(ui32 connectionId)
{
	assert(this->mType.includes(EType::eServer));

	auto iterClient = this->mClients.find(connectionId);
	auto netId = iterClient->second;
	this->mClients.erase(iterClient);
	this->mNetIdToConnection.erase(this->mNetIdToConnection.find(netId));
	this->onConnectionClosed.execute(this, connectionId, netId);

	this->mUnusedNetIds.insert(netId);
	
	packet::ClientStatus::create()
		->setStatus(EClientStatus::eDisconnected)
		.setIsSelf(false).setNetId(netId)
		.broadcast({ connectionId });
	if (this->type().includes(EType::eClient))
	{
		this->onClientPeerStatusChanged.execute(this, netId, EClientStatus::eDisconnected);
	}

	auto* pInterface = as<ISteamNetworkingSockets>(this->mpInternal);
	pInterface->CloseConnection(connectionId, 0, nullptr, false);

	return netId;
}
