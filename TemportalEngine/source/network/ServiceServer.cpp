#include "network\ServiceServer.hpp"

using namespace network;

ServiceServer::ServiceServer() : Service()
{
}

void ServiceServer::initialize(ui16 const port, ui16 const maxClients)
{
	this->mpNetworkInterface->initServer(port, maxClients);
}
