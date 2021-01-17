#include "network/packet/NetworkPacketClientStatus.hpp"

#include "network/NetworkInterface.hpp"

using namespace network;
using namespace network::packet;

DEFINE_PACKET_TYPE(ClientStatus)

ClientStatus::ClientStatus()
	: Packet(EPacketFlags::eReliable)
{
}

ClientStatus& ClientStatus::setIsSelf(bool bIsSelf)
{
	this->mbIsSelf = bIsSelf;
	return *this;
}

ClientStatus& ClientStatus::setNetId(ui32 netId)
{
	this->mNetId = netId;
	return *this;
}

ClientStatus& ClientStatus::setStatus(EClientStatus status)
{
	this->mStatus = status;
	return *this;
}

std::string to_string(EClientStatus status)
{
	switch (status)
	{
	case EClientStatus::eAuthenticating: return "authenticating";
	case EClientStatus::eConnected: return "connected";
	case EClientStatus::eDisconnected: return "disconnected";
	default: return "invalid";
	}
}

void ClientStatus::write(Buffer &archive) const
{
	Packet::write(archive);
	network::write(archive, "isSelf", this->mbIsSelf);
	network::write(archive, "netId", this->mNetId);
	archive.setNamed("status", to_string(this->mStatus));
	archive.writeRaw(this->mStatus);
}

void ClientStatus::read(Buffer &archive)
{
	Packet::read(archive);
	network::read(archive, "isSelf", this->mbIsSelf);
	network::read(archive, "netId", this->mNetId);
	archive.readRaw(this->mStatus);
	archive.setNamed("status", to_string(this->mStatus));
}

void ClientStatus::process(network::Interface *pInterface)
{
	assert((EType)pInterface->type() == EType::eClient);
	// Server has confirmed data for this client
	if (this->mbIsSelf)
	{
		if (this->mStatus == EClientStatus::eAuthenticating)
		{
			network::logger().log(LOG_INFO, "Received network id %i", this->mNetId);
			pInterface->onNetIdReceived.execute(pInterface, this->mNetId);
		}
		else if (this->mStatus == EClientStatus::eConnected)
		{
			pInterface->OnDedicatedClientAuthenticated.execute(pInterface, this->mNetId);
		}
	}
	// A new client has arrived
	else
	{
		if (this->mStatus == EClientStatus::eConnected)
		{
			network::logger().log(LOG_INFO, "A peer client has joined with the network id %i", this->mNetId);
			pInterface->onClientPeerStatusChanged.execute(pInterface, this->mNetId, EClientStatus::eConnected);
		}
		else if (this->mStatus == EClientStatus::eDisconnected)
		{
			network::logger().log(LOG_INFO, "Network peer %i has disconnected", this->mNetId);
			pInterface->onClientPeerStatusChanged.execute(pInterface, this->mNetId, EClientStatus::eDisconnected);
		}
	}
}
