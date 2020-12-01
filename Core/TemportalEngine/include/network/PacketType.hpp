#ifndef NETWORK_PACKETTYPE_HPP
#define NETWORK_PACKETTYPE_HPP

#include "TemportalEnginePCH.hpp"

// TODO: Organize Headers

#include "network/PacketInternal.hpp"

NS_NETWORK

namespace packets
{
	namespace server
	{
		static PacketInternal::Id NewIncomingConnection;
	};
	namespace client
	{
		static PacketInternal::Id ConnectionRequestAccepted;
		static PacketInternal::Id ConnectionRequestRejected;
	};
};

NS_END

#endif