#pragma once

#include "network/NetworkPacket.hpp"

NS_NETWORK
NS_PACKET

class ChatMessage : public Packet
{
	DECLARE_PACKET_TYPE(ChatMessage)

public:
	ChatMessage();
	static void broadcastServerMessage(std::string const& msg);

	ChatMessage& setMessage(std::string const& msg);

	void write(Buffer &archive) const override;
	void read(Buffer &archive) override;
	void process(Interface *pInterface) override;

private:
	bool mbIsServerMessage;
	// empty when sent from client.
	// filled by server when broadcasting based on the connection id.
	ui32 mSenderNetId;
	// the chat message sent from the client.
	std::string mMsg;

	void writeToLog(utility::Flags<EType> netType) const;
	void writeToClientChat() const;

};

NS_END
NS_END
