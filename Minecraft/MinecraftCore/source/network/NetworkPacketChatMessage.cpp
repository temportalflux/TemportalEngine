#include "network/NetworkPacketChatMessage.hpp"

#include "Engine.hpp"
#include "network/NetworkInterface.hpp"

using namespace network;

logging::Logger CHAT_LOG = DeclareLog("Chat");

DEFINE_PACKET_TYPE(PacketChatMessage, mData)

PacketChatMessage::PacketChatMessage()
	: Packet(EPacketFlags::eReliable)
{
}

PacketChatMessage& PacketChatMessage::setMessage(std::string const& msg)
{
	assert(msg.length() * sizeof(char) < sizeof(this->mData.msg));
	strcpy_s(this->mData.msg, msg.c_str());
	return *this;
}

void PacketChatMessage::process(network::Interface *pInterface)
{
	switch (pInterface->type())
	{
	case EType::eServer:
	{
		// TODO Store user's name in client data for look up here
		strcpy_s(this->mData.senderName, pInterface->findIdentity(this->connection()).name.c_str());
		CHAT_LOG.log(LOG_INFO, "%s: %s", this->mData.senderName, this->mData.msg);
		this->broadcast();
		break;
	}
	case EType::eClient:
	{
		CHAT_LOG.log(LOG_INFO, "%s: %s", this->mData.senderName, this->mData.msg);
		break;
	}
	default: break;
	}
}
