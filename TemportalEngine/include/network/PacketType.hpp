#ifndef NETWORK_PACKETTYPE_HPP
#define NETWORK_PACKETTYPE_HPP

#include "Namespace.h"
#include "Api.h"
#include "network/Packet.hpp"

NS_NETWORK

namespace packets
{
	namespace server
	{
		static Packet::Id NewIncomingConnection;
	};
	namespace client
	{
		static Packet::Id ConnectionRequestAccepted;
		static Packet::Id ConnectionRequestRejected;
	};
};

NS_END

#endif