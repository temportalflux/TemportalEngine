#include "network\ServiceClient.hpp"

using namespace network;

ServiceClient::ServiceClient() : Service()
{
}

void ServiceClient::initialize()
{
	this->mpNetworkInterface->initClient();
}

void ServiceClient::connectToServer(char const * address, ui16 const port)
{
	this->mpNetworkInterface->connectToServer(address, port);
}
