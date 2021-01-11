#pragma once

#include "network/NetworkPacket.hpp"

NS_NETWORK

class PacketChatMessage : public Packet
{
	DECLARE_PACKET_TYPE(PacketChatMessage)

public:
	PacketChatMessage();

	PacketChatMessage& setMessage(std::string const& msg);

	void process(Interface *pInterface) override;

private:
	struct : Packet::Data
	{
		// empty when sent from client.
		// filled by server when broadcasting based on the connection id.
		char senderName[32];
		// the chat message sent from the client.
		char msg[128];
	} mData;

};

NS_END
