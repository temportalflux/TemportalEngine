#pragma once

#include "network/NetworkCore.hpp"
#include "Delegate.hpp"

#include "network/NetworkAddress.hpp"
#include "network/NetworkPacketTypeRegistry.hpp"

NS_NETWORK
namespace packet { class Packet; };

class Interface
{

public:
	Interface();

	PacketTypeRegistry& packetTypes() { return this->mPacketRegistry; }

	Interface& setType(EType type);
	EType type() const { return this->mType; }
	
	Interface& setAddress(Address const& address);

	ExecuteDelegate<void(Interface*, ui32 connection, ui32 netId)> onConnectionEstablished;
	ExecuteDelegate<void(Interface*, ui32 netId)> onNetIdReceived;
	ExecuteDelegate<void(Interface*, ui32 netId)> onClientPeerJoined;

	void start();
	bool hasConnection() const;
	void update(f32 deltaTime);
	void stop();

	ui32 connection() const { return this->mConnection; }
	void sendPackets(ui32 connection, std::vector<std::shared_ptr<packet::Packet>> const& packets);
	void broadcastPackets(std::vector<std::shared_ptr<packet::Packet>> const& packets, std::set<ui32> except = {});

	ui32 getNetIdFor(ui32 connection) const;

private:
	PacketTypeRegistry mPacketRegistry;

	EType mType;
	// For clients: the address and port of the server to connect to
	// For servers: localhost + the port to listen on
	Address mAddress;

	void* /*ISteamNetworkingSockets*/ mpInternal;
	
	// For clients: the network connection to a server
	// For servers: the listen socket
	ui32 mConnection;
	ui32 mServerPollGroup;

	std::map<ui32 /*connectionId*/, ui32 /*netId*/> mClients;
	std::set<ui32> mUnusedNetIds;
	ui32 nextNetworkId();

	std::vector<std::shared_ptr<packet::Packet>> mReceivedPackets;
	void pollIncomingMessages();
	
public:
	void onServerConnectionStatusChanged(void* pInfo);
	void onClientConnectionStatusChanged(void* pInfo);

};

NS_END
