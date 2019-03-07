#include "network/client/ServiceClient.hpp"

// Engine ---------------------------------------------------------------------
#include "Engine.hpp"
#include "network/PacketType.hpp"
#include "network/client/PacketConnectionAccepted.hpp"

// ----------------------------------------------------------------------------

using namespace network;

// ----------------------------------------------------------------------------

ServiceClient::ServiceClient() : Service()
{
}

void ServiceClient::initialize()
{
	this->mpNetworkInterface->initClient();

	if (!registerPacket(PacketConnectionAccepted::Identification, PacketConnectionAccepted()))
	{
		LogEngine(logging::ECategory::LOGERROR, "Could not register packet client::ConnectionRequestAccepted");
	}

	/*
	if (!registerPacket(network::packets::client::ConnectionRequestRejected, [](PacketInternal::Id const &id, void* const data) {
		LogEngine(logging::ECategory::LOGINFO, "Executed ConnectionRequestRejected");
	}))
	{
		LogEngine(logging::ECategory::LOGERROR, "Could not register packet client::ConnectionRequestRejected");
	}*/
}

void ServiceClient::connectToServer(char const * address, ui16 const port)
{
	this->mpNetworkInterface->connectToServer(address, port);
}
