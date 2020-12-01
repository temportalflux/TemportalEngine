#include "network/server/PacketNewIncomingConnection.hpp"

// Engine ---------------------------------------------------------------------
#include "Engine.hpp"

// ----------------------------------------------------------------------------

using namespace network;

// ----------------------------------------------------------------------------

RegistryIdentifier PacketNewIncomingConnection::Identification = std::nullopt;

Packet* PacketNewIncomingConnection::operator()(void* data)
{
	// TODO: Optimize with memory manager
	return new PacketNewIncomingConnection();
}

void PacketNewIncomingConnection::execute() const
{
	LogEngineInfo("Found new incoming connection");
}
