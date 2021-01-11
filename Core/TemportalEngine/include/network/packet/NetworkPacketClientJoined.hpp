#pragma once

#include "network/NetworkPacket.hpp"

NS_NETWORK
NS_PACKET

class ClientJoined : public Packet
{
	DECLARE_PACKET_TYPE(ClientJoined)

public:
	ClientJoined();

	ClientJoined& setIsSelf(bool bIsSelf);
	ClientJoined& setNetId(ui32 netId);

	void process(Interface *pInterface) override;

private:
	struct : Packet::Data
	{
		bool bIsSelf;
		ui32 netId;
	} mData;

};

NS_END
NS_END
