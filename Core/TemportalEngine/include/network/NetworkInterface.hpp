#pragma once

#include "network/NetworkCore.hpp"
#include "Delegate.hpp"

#include "network/NetworkAddress.hpp"
#include "network/NetworkPacketTypeRegistry.hpp"
#include "utility/Flags.hpp"

NS_NETWORK
namespace packet { class Packet; };

using ConnectionId = ui32;

class Interface
{

public:
	Interface();

	PacketTypeRegistry& packetTypes() { return this->mPacketRegistry; }

	Interface& setType(utility::Flags<network::EType> type);
	utility::Flags<network::EType> const& type() const;
	
	Interface& setAddress(Address const& address);

	BroadcastDelegate<void(Interface*)> onNetworkStarted;
	BroadcastDelegate<void(Interface*)> onNetworkStopped;

	BroadcastDelegate<void(Interface*, ConnectionId connId)> onConnectionEstablished;
	ExecuteDelegate<void(
		Interface*, ConnectionId connId,
		std::optional<network::Identifier> netId
	)> onConnectionClosed;

	ExecuteDelegate<void(Interface*, network::Identifier netId)> OnClientAuthenticatedOnServer;
	ExecuteDelegate<void(Interface*, network::Identifier netId)> OnClientAuthenticatedOnClient;
	ExecuteDelegate<void(Interface*, network::Identifier netId, EClientStatus status)> onClientPeerStatusChanged;
	BroadcastDelegate<void(Interface*, network::Identifier netId)> OnDedicatedClientDisconnected;

	void start();
	bool hasConnection() const;
	void update(f32 deltaTime);
	void stop();

	ConnectionId connection() const { return this->mConnection; }
	void sendPackets(
		std::set<ConnectionId> connections,
		std::vector<std::shared_ptr<packet::Packet>> const& packets,
		std::set<ConnectionId> except = {}
	);

	std::set<ConnectionId> connections() const;
	std::set<network::Identifier> connectedClientNetIds() const;
	network::Identifier getNetIdFor(ConnectionId connId) const;
	ConnectionId getConnectionFor(network::Identifier netId) const;
	void closeConnection(ConnectionId connId);

	void markClientAuthenticated(ConnectionId connId);

private:
	PacketTypeRegistry mPacketRegistry;

	utility::Flags<network::EType> mType;
	// For dedicated clients: the address and port of the server to connect to
	// For dedicated or integrated servers: local-host + the port to listen on
	Address mAddress;

	void* /*ISteamNetworkingSockets*/ mpInternal;
	
	// For dedicated clients: the network connection to a server
	// For dedicated or integrated servers: the listen socket
	ConnectionId mConnection;
	ui32 mServerPollGroup;

	std::set<ConnectionId> mConnections;
	std::map<ConnectionId, network::Identifier> mClients;
	std::map<network::Identifier, ConnectionId> mNetIdToConnection;
	std::set<network::Identifier> mUnusedNetIds;
	network::Identifier nextNetworkId();
	void addClient(ConnectionId connId, network::Identifier netId);

	struct RecievedPacket
	{
		std::shared_ptr<packet::Packet> packet;
		std::optional<std::string> debugLog;
	};
	std::vector<RecievedPacket> mReceivedPackets;
	void pollIncomingMessages();
	
public:
	void onServerConnectionStatusChanged(void* pInfo);
	void onClientConnectionStatusChanged(void* pInfo);

};

NS_END
