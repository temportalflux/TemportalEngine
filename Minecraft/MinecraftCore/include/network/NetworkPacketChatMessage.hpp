#pragma once

#include "network/NetworkPacket.hpp"

NS_NETWORK
NS_PACKET

class ChatMessage : public Packet
{
	DECLARE_PACKET_TYPE(ChatMessage)

public:
	ChatMessage();

	ChatMessage& setMessage(std::string const& msg);

	void process(Interface *pInterface) override;

private:
	struct : Packet::Data
	{
		// empty when sent from client.
		// filled by server when broadcasting based on the connection id.
		ui32 senderNetId;
		// the chat message sent from the client.
		char msg[128];
	} mData;

};

NS_END
NS_END
