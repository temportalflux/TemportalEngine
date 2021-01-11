#pragma once

#include "network/NetworkCore.hpp"

#include "network/NetworkAddress.hpp"
#include "network/NetworkPacketTypeRegistry.hpp"

NS_NETWORK
class Packet;

class Interface
{

public:
	Interface();

	PacketTypeRegistry& packetTypes() { return this->mPacketRegistry; }

	Interface& setType(EType type);
	EType type() const { return this->mType; }
	
	Interface& setAddress(Address const& address);

	void start();
	bool hasConnection() const;
	void update(f32 deltaTime);
	void stop();

	ui32 connection() const { return this->mConnection; }
	void sendPackets(ui32 connection, std::vector<std::shared_ptr<Packet>> const& packets);
	void broadcastPackets(std::vector<std::shared_ptr<Packet>> const& packets);

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

	std::set<ui32> mClientIds;

	std::vector<std::shared_ptr<Packet>> mReceivedPackets;

	void pollIncomingMessages();

public:
	void onServerConnectionStatusChanged(void* pInfo);
	void onClientConnectionStatusChanged(void* pInfo);

};

NS_END
