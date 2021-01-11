#pragma once

#include "network/NetworkPacket.hpp"

NS_NETWORK

class PacketSetName : public Packet
{
	DECLARE_PACKET_TYPE(PacketSetName)

public:
	PacketSetName();

	PacketSetName& setName(std::string const& name);

	void process(Interface *pInterface) override;

private:
	struct : Packet::Data
	{
		char name[32];
	} mData;

};

NS_END
