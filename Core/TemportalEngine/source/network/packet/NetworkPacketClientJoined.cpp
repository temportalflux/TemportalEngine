#include "network/packet/NetworkPacketClientJoined.hpp"

#include "network/NetworkInterface.hpp"

using namespace network;
using namespace network::packet;

DEFINE_PACKET_TYPE(ClientJoined, mData)

ClientJoined::ClientJoined()
	: Packet(EPacketFlags::eReliable)
	, mData({})
{
}

ClientJoined& ClientJoined::setIsSelf(bool bIsSelf)
{
	this->mData.bIsSelf = bIsSelf;
	return *this;
}

ClientJoined& ClientJoined::setNetId(ui32 netId)
{
	this->mData.netId = netId;
	return *this;
}

void ClientJoined::process(network::Interface *pInterface)
{
	assert(pInterface->type() == EType::eClient);
	// Server has confirmed data for this client
	if (this->mData.bIsSelf)
	{
		network::logger().log(LOG_INFO, "Received network id %i", this->mData.netId);
		pInterface->onNetIdReceived.execute(pInterface, this->mData.netId);
	}
	// A new client has arrived
	else
	{
		network::logger().log(LOG_INFO, "A peer client has joined with the network id %i", this->mData.netId);
		pInterface->onClientPeerJoined.execute(pInterface, this->mData.netId);
	}
}
