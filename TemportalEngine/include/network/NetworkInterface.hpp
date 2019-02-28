#ifndef NETWORK_INTERFACE_HPP
#define NETWORK_INTERFACE_HPP

#include "Namespace.h"
#include "Api.h"
#include "network/PacketQueue.hpp"
#include "network/Event.hpp"

NS_ENGINE
class Engine;
NS_END

NS_NETWORK

class TEMPORTALENGINE_API NetworkInterface
{
	friend class engine::Engine;
	
private:
	
	bool mIsServer;
	void* mpPeerInterface;
	PacketQueue mpQueue[1];

#pragma region RunOps

	// Startup the server interface
	void initServer(const ui16 port, const ui16 maxClients);

	// Startup the client interface
	void initClient();

	// Connect the interface to its destination
	void connectToServer(char const *address, const ui16 port);

	// Shutdown the peer interface
	void disconnect();

#pragma endregion

#pragma region Status

	// Return the IP string from the peer
	char const * getIP() const;

	void queryAddress() const;

	// Returns true if the network interface (RakNet thread) is active
	bool isActive() const;

	bool isServer() const;

#pragma endregion

#pragma region Packets

	bool fetchPacket();
	// Cache all incoming packets (should be run regularly)
	void fetchAllPackets();
	void processAllPackets();
	void processPacket(Packet const &packet);

#pragma endregion

	/*
	//! Send packet data over RakNet
	// TODO: Encapsulation Leek
	void sendTo(Data data, DataSize size,
		RakNet::SystemAddress *address,
		PacketPriority *priority, PacketReliability *reliability,
		char channel, bool broadcast, bool timestamp, const TimeStamp *timestampInfo = NULL
	);

	//! Handle sending struct packets to RakNet address
	// TODO: Encapsulation Leek
	template <typename T>
	void sendTo(T packet,
		RakNet::SystemAddress *address,
		PacketPriority *priority, PacketReliability *reliability,
		char channel, bool broadcast, bool timestamp
	)
	{
		// Package up the packet
		Data data = (Data)(&packet);
		DataSize size = sizeof(packet);
		// Send via RakNet
		this->sendTo(data, size, address, priority, reliability, channel, broadcast, timestamp);
	}

	//! Handle sending struct packets to RakNet address
	// TODO: Encapsulation Leek
	template <typename T>
	void sendTo(T packet, RakNet::SystemAddress *address)
	{
		this->sendTo(packet, address, HIGH_PRIORITY, RELIABLE_ORDERED, 0, false, true);
	}
	//*/

	// Poll the next cached packet
	// Returns true if a packet was found;
	//bool pollPackets(Packet *&nextPacket);

	// Write the time stamp to a buffer
	//int writeTimestamps(char *buffer, const RakNet::Time &time1, const RakNet::Time &time2);
	// Read the time stamp from a buffer
	//int readTimestamps(const char *buffer, RakNet::Time &time1, RakNet::Time &time2);

protected:

	NetworkInterface();

public:
	~NetworkInterface();

	/*template <typename TData>
	void registerPacket(ui16 id)
	{
		sizeof(TData);
	}*/

	static void runThread(void* pInterface);

};

NS_END

#endif