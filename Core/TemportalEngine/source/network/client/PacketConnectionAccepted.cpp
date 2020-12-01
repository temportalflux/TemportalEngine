#include "network/client/PacketConnectionAccepted.hpp"

// Engine ---------------------------------------------------------------------
#include "Engine.hpp"

// ----------------------------------------------------------------------------

using namespace network;

// ----------------------------------------------------------------------------

RegistryIdentifier PacketConnectionAccepted::Identification = std::nullopt;

Packet* PacketConnectionAccepted::operator()(void* data)
{
	// TODO: Optimize with memory manager
	return new PacketConnectionAccepted();
}

void PacketConnectionAccepted::execute() const
{
	LogEngineInfo("Connection was accepted");
}
