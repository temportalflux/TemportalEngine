#include "network/packet/NetworkPacketClientStatus.hpp"

#include "network/NetworkInterface.hpp"

using namespace network;
using namespace network::packet;

DEFINE_PACKET_TYPE(ClientStatus, mData)

ClientStatus::ClientStatus()
	: Packet(EPacketFlags::eReliable)
	, mData({})
{
}

ClientStatus& ClientStatus::setIsSelf(bool bIsSelf)
{
	this->mData.bIsSelf = bIsSelf;
	return *this;
}

ClientStatus& ClientStatus::setNetId(ui32 netId)
{
	this->mData.netId = netId;
	return *this;
}

ClientStatus& ClientStatus::setStatus(EClientStatus status)
{
	this->mData.status = status;
	return *this;
}

void ClientStatus::process(network::Interface *pInterface)
{
	assert((EType)pInterface->type() == EType::eClient);
	// Server has confirmed data for this client
	if (this->mData.bIsSelf)
	{
		assert(this->mData.status == EClientStatus::eConnected);
		network::logger().log(LOG_INFO, "Received network id %i", this->mData.netId);
		pInterface->onNetIdReceived.execute(pInterface, this->mData.netId);
	}
	// A new client has arrived
	else if (this->mData.status == EClientStatus::eConnected)
	{
		network::logger().log(LOG_INFO, "A peer client has joined with the network id %i", this->mData.netId);
		pInterface->onClientPeerStatusChanged.execute(pInterface, this->mData.netId, EClientStatus::eConnected);
	}
	else if (this->mData.status == EClientStatus::eDisconnected)
	{
		network::logger().log(LOG_INFO, "Network peer %i has disconnected", this->mData.netId);
		pInterface->onClientPeerStatusChanged.execute(pInterface, this->mData.netId, EClientStatus::eDisconnected);
	}
}
