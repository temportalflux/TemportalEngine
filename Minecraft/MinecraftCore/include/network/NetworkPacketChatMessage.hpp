#pragma once

#include "network/NetworkPacket.hpp"

NS_NETWORK

class PacketChatMessage : public Packet
{

public:
	static ui32 TypeId;

	PacketChatMessage();

	static std::shared_ptr<PacketChatMessage> create();
	Packet::Data* data() override { return dynamic_cast<Packet::Data*>(&this->mData); }
	ui32 dataSize() const override { return sizeof(this->mData); }

	PacketChatMessage& setMessage(std::string const& msg);

	void process(network::EType netType) override;

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
