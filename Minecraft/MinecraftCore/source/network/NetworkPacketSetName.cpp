#include "network/NetworkPacketSetName.hpp"

#include "Engine.hpp"
#include "network/NetworkInterface.hpp"
#include "game/GameInstance.hpp"

using namespace network;
using namespace network::packet;

DEFINE_PACKET_TYPE(SetName, mData)

SetName::SetName()
	: Packet(EPacketFlags::eReliable)
{
}

SetName& SetName::setName(std::string const& name)
{
	assert(name.length() * sizeof(char) < sizeof(this->mData.name));
	strcpy_s(this->mData.name, name.c_str());
	return *this;
}

void SetName::process(Interface *pInterface)
{
	switch (pInterface->type())
	{
	case EType::eServer:
	{
		this->mData.netId = pInterface->getNetIdFor(this->connection());
		network::logger().log(LOG_INFO, "Received alias %s for network-id %i", this->mData.name, this->mData.netId);
		game::Game::Get()->findConnectedUser(this->mData.netId).name = this->mData.name;
		this->broadcast();
		break;
	}
	case EType::eClient:
	{
		if (this->connection() != pInterface->connection())
		{
			network::logger().log(LOG_INFO, "Received alias %s for network-id %i", this->mData.name, this->mData.netId);
			game::Game::Get()->findConnectedUser(this->mData.netId).name = this->mData.name;
		}
		break;
	}
	default: break;
	}
}
