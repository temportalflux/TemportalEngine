#include "network/NetworkPacketSetName.hpp"

#include "Engine.hpp"
#include "network/NetworkInterface.hpp"

using namespace network;

DEFINE_PACKET_TYPE(PacketSetName, mData)

PacketSetName::PacketSetName()
	: Packet(EPacketFlags::eReliable)
{
}

PacketSetName& PacketSetName::setName(std::string const& name)
{
	assert(name.length() * sizeof(char) < sizeof(this->mData.name));
	strcpy_s(this->mData.name, name.c_str());
	return *this;
}

void PacketSetName::process(Interface *pInterface)
{
	assert(pInterface->type() == EType::eServer);
	network::logger().log(LOG_INFO, "Received alias %s for connection %i", this->mData.name, this->connection());
	pInterface->findIdentity(this->connection()).name = this->mData.name;
}
