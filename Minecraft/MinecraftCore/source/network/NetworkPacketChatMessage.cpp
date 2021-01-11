#include "network/NetworkPacketChatMessage.hpp"

#include "Engine.hpp"

using namespace network;

logging::Logger CHAT_LOG = DeclareLog("Chat");

ui32 PacketChatMessage::TypeId = 0;

std::shared_ptr<PacketChatMessage> PacketChatMessage::create()
{
	return std::make_shared<PacketChatMessage>();
}

PacketChatMessage::PacketChatMessage()
	: Packet(EPacketFlags::eReliable)
{
	this->data()->packetTypeId = PacketChatMessage::TypeId;
}

PacketChatMessage& PacketChatMessage::setMessage(std::string const& msg)
{
	assert(msg.length() * sizeof(char) < sizeof(this->mData.msg));
	strcpy_s(this->mData.msg, msg.c_str());
	return *this;
}

void PacketChatMessage::process(network::EType netType)
{
	switch (netType)
	{
	case EType::eServer:
	{
		// TODO Store user's name in client data for look up here
		strcpy_s(this->mData.senderName, "unknown");
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
