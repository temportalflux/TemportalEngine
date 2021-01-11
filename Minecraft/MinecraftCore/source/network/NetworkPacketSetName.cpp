#include "network/NetworkPacketSetName.hpp"

#include "Engine.hpp"
#include "network/NetworkInterface.hpp"
#include "network/NetworkPacketChatMessage.hpp"
#include "game/GameInstance.hpp"

using namespace network;
using namespace network::packet;

DEFINE_PACKET_TYPE(SetName, mData)

SetName::SetName()
	: Packet(EPacketFlags::eReliable)
{
}

SetName& SetName::setNetId(ui32 netId)
{
	this->mData.netId = netId;
	return *this;
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
		this->setNetId(pInterface->getNetIdFor(this->connection()));
		network::logger().log(LOG_INFO, "Received alias %s for network-id %i", this->mData.name, this->mData.netId);
		
		auto& userId = game::Game::Get()->findConnectedUser(this->mData.netId);
		std::string oldName = userId.name;
		userId.name = this->mData.name;
		this->broadcast();
		
		ChatMessage::broadcastServerMessage(
			oldName.length() == 0
			? utility::formatStr("%s has joined the server.", this->mData.name)
			: utility::formatStr("%s is now named %s", oldName.c_str(), this->mData.name)
		);
		break;
	}
	case EType::eClient:
	{
		network::logger().log(LOG_INFO, "Received alias %s for network-id %i", this->mData.name, this->mData.netId);
		game::Game::Get()->findConnectedUser(this->mData.netId).name = this->mData.name;
		break;
	}
	default: break;
	}
}
