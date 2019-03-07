#include "network/server/ServiceServer.hpp"

// Engine ---------------------------------------------------------------------
#include "Engine.hpp"
#include "network/PacketType.hpp"
#include "network/server/PacketNewIncomingConnection.hpp"

// ----------------------------------------------------------------------------

using namespace network;

// ----------------------------------------------------------------------------

ServiceServer::ServiceServer() : Service()
{
}

void ServiceServer::initialize(ui16 const port, ui16 const maxClients)
{
	this->mpNetworkInterface->initServer(port, maxClients);

	if (!registerPacket(PacketNewIncomingConnection::Identification, PacketNewIncomingConnection()))
	{
		LogEngine(logging::ECategory::LOGERROR, "Could not register packet server::NewIncomingConnection");
	}
}
