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
		.addType<network::packet::EVCSReplicate>()
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
			// Broadcast connection to itself (client on top of server)
			this->onConnectionEstablished.broadcast(this, this->mConnection);
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

		network::logger().log(
			LOG_INFO, "Accepted connection request from %s. Awaiting authentication.",
			data->m_info.m_szConnectionDescription
		);

		this->onConnectionEstablished.broadcast(this, data->m_hConn);
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
				this->OnDedicatedClientDisconnected.broadcast(this, this->mClients.find(data->m_hConn)->second);
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
		this->onConnectionEstablished.broadcast(this, data->m_hConn);
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
		this->OnDedicatedClientDisconnected.broadcast(this, 0);
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

	for (auto idxPacket = 0; idxPacket < packets.size(); ++idxPacket)
	{
		auto pPacket = packets[idxPacket];
		auto buffer = network::Buffer(this->type(), nullptr, 0, true);
		pPacket->write(buffer);

		if (bIsLogEnabled)
		{
			pPacket->appendDebugLogHeader(debugLog);
			buffer.stringify(debugLog);
			if (idxPacket < packets.size() - 1)
				debugLog << '\n';
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

std::set<ConnectionId> Interface::connections() const
{
	return this->mConnections;
}

void Interface::addClient(ConnectionId connId, network::Identifier netId)
{
	this->mConnections.insert(connId);
	this->mClients.insert(std::make_pair(connId, netId));
	this->mNetIdToConnection.insert(std::make_pair(netId, connId));
}

std::set<network::Identifier> Interface::connectedClientNetIds() const
{
	auto ids = std::set<network::Identifier>();
	for (auto const&[connection, netId] : this->mClients)
	{
		ids.insert(netId);
	}
	return ids;
}

network::Identifier Interface::nextNetworkId()
{
	if (this->mUnusedNetIds.size() > 0)
	{
		auto id = *this->mUnusedNetIds.begin();
		this->mUnusedNetIds.erase(this->mUnusedNetIds.begin());
		return id;
	}
	return (ui32)this->mConnections.size();
}

network::Identifier Interface::getNetIdFor(ConnectionId connId) const
{
	assert(this->type().includes(EType::eServer));
	auto iter = this->mClients.find(connId);
	assert(iter != this->mClients.end());
	return iter->second;
}

ConnectionId Interface::getConnectionFor(network::Identifier netId) const
{
	assert(this->type().includes(EType::eServer));
	auto iter = this->mNetIdToConnection.find(netId);
	assert(iter != this->mNetIdToConnection.end());
	return iter->second;
}

void Interface::markClientAuthenticated(ConnectionId connId)
{
	auto const netId = this->nextNetworkId();

	network::logger().log(
		LOG_VERBOSE, "Assigning connectionId(%u) the netId(%u)",
		connId, netId
	);

	this->addClient(connId, netId);

	// Tell the client its own net id
	packet::ClientStatus::create()
		->setStatus(EClientStatus::eConnected)
		.setIsSelf(true).setNetId(netId)
		.send(connId);

	// Tell all other clients that a client has joined
	packet::ClientStatus::create()
		->setStatus(EClientStatus::eConnected)
		.setIsSelf(false).setNetId(netId)
		.broadcast({ connId });

	// Tell the newly connected client of the net ids of other already joined clients
	for (auto const& [otherConnectionId, otherNetId] : this->mClients)
	{
		if (otherNetId == netId) continue;
		packet::ClientStatus::create()
			->setStatus(EClientStatus::eConnected)
			.setIsSelf(false).setNetId(otherNetId)
			.send(connId);
	}

	this->OnClientAuthenticatedOnServer.execute(this, netId);
}

void Interface::closeConnection(ConnectionId connId)
{
	assert(this->mType.includes(EType::eServer));

	// Its possible for this function to run on a connection
	// that has not authenticated (so there will be no netId or client data).
	std::optional<network::Identifier> netId = std::nullopt;

	auto iterConn = this->mConnections.find(connId);
	if (iterConn != this->mConnections.end()) this->mConnections.erase(iterConn);

	auto iterClient = this->mClients.find(connId);
	if (iterClient != this->mClients.end())
	{
		netId = iterClient->second;
		this->mClients.erase(iterClient);
	}
	
	// If the connection had been authenticated, then they have a net-id and we should clean up.
	if (netId.has_value())
	{
		// Their net id is now unused
		this->mUnusedNetIds.insert(netId.value());
		// And we know they will have an entry in the net-id to connection map
		this->mNetIdToConnection.erase(this->mNetIdToConnection.find(netId.value()));

		// Tell all other clients that the user is disconnecting
		packet::ClientStatus::create()
			->setStatus(EClientStatus::eDisconnected)
			.setIsSelf(false).setNetId(netId.value())
			.broadcast({ connId });	
	}

	// Actually close the network connection
	auto* pInterface = as<ISteamNetworkingSockets>(this->mpInternal);
	pInterface->CloseConnection(connId, 0, nullptr, false);

	this->onConnectionClosed.execute(this, connId, netId);
}
