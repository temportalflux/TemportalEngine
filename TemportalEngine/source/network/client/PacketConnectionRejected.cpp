#include "network/client/PacketConnectionRejected.hpp"

// Engine ---------------------------------------------------------------------
#include "Engine.hpp"

// ----------------------------------------------------------------------------

using namespace network;

// ----------------------------------------------------------------------------

RegistryIdentifier PacketConnectionRejected::Identification = std::nullopt;

Packet* PacketConnectionRejected::operator()(void* data)
{
	// TODO: Optimize with memory manager
	return new PacketConnectionRejected();
}

void PacketConnectionRejected::execute() const
{
	LogEngineInfo("Connection was rejected");
}
