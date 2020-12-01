#include "network/server/ServiceServer.hpp"

// Engine ---------------------------------------------------------------------
#include "Engine.hpp"
#include "network/PacketType.hpp"

// ----------------------------------------------------------------------------

using namespace network;

// ----------------------------------------------------------------------------

ServiceServer::ServiceServer() : Service()
{
}

void ServiceServer::initialize(ui16 const port, ui16 const maxClients)
{
	this->registerPackets();
	this->mpNetworkInterface->initServer(port, maxClients);
}
