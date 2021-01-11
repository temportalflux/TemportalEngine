#pragma once

#include "network/NetworkPacket.hpp"

NS_NETWORK
NS_PACKET

class SetName : public Packet
{
	DECLARE_PACKET_TYPE(SetName)

public:
	SetName();

	SetName& setName(std::string const& name);

	void process(Interface *pInterface) override;

private:
	struct : Packet::Data
	{
		// empty when sent from clients
		// populated by server when broadcasted to peers
		ui32 netId;
		char name[32];
	} mData;

};

NS_END
NS_END
