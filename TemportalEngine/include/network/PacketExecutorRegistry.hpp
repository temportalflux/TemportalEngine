#ifndef NETWORK_PACKET_EXECUTORREGISTRY_HPP
#define NETWORK_PACKET_EXECUTORREGISTRY_HPP

#include "Namespace.h"
#include "Api.h"
#include "Packet.hpp"
#include <optional>

NS_NETWORK

class Service;

class TEMPORTALENGINE_API PacketExecutorRegistry
{
	friend class network::Service;

public:
	typedef void(*DelegatePacketExecutor)(Packet::Id const /*packetId*/, void* const /*data*/);

private:

	DelegatePacketExecutor mPacketExecutors[MAX_PACKET_COUNT];
	Packet::Id mPacketTypeCount;

	PacketExecutorRegistry();

	// Returns the packet ID if it was able to register the packet
	std::optional<Packet::Id> registerPacket(DelegatePacketExecutor pCallback);

	std::optional<DelegatePacketExecutor> getPacketExecutor(Packet::Id const &packetId);

};

NS_END

#endif