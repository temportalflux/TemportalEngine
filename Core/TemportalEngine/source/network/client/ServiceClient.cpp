#include "network/client/ServiceClient.hpp"

// Engine ---------------------------------------------------------------------
#include "Engine.hpp"
#include "network/PacketType.hpp"

// ----------------------------------------------------------------------------

using namespace network;

// ----------------------------------------------------------------------------

ServiceClient::ServiceClient() : Service()
{
}

void ServiceClient::initialize()
{
	this->registerPackets();
	this->mpNetworkInterface->initClient();
}

void ServiceClient::connectToServer(char const * address, ui16 const port)
{
	this->mpNetworkInterface->connectToServer(address, port);
}
