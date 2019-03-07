#ifndef TE_NETWORK_PACKETREGISTRY_HPP
#define TE_NETWORK_PACKETREGISTRY_HPP
#pragma warning(push)
#pragma warning(disable:4251) // disable STL warnings in dll

// PCH ------------------------------------------------------------------------
#include "TemportalEnginePCH.hpp"

// Libraries ------------------------------------------------------------------
#include <optional>
#include <functional>

// Engine ---------------------------------------------------------------------
#include "registry/Registry.hpp"
#include "network/common/Packet.hpp"
#include "network/NetworkBudget.hpp"

// ----------------------------------------------------------------------------
NS_NETWORK

class Service;

class TEMPORTALENGINE_API PacketRegistry : public utility::Registry<std::function<Packet*(void*)>, MAX_PACKET_COUNT>
{
	friend class network::Service;

private:
	PacketRegistry();

public:

	std::optional<Packet*> getPacketFrom(RegistryIdentifier const & id, void * data) const;

};

NS_END

#pragma warning(pop)

#endif