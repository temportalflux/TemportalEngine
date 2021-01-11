#pragma once

#include "network/NetworkPacket.hpp"

NS_NETWORK
NS_PACKET

class ClientStatus : public Packet
{
	DECLARE_PACKET_TYPE(ClientStatus)

public:
	ClientStatus();

	ClientStatus& setIsSelf(bool bIsSelf);
	ClientStatus& setNetId(ui32 netId);
	ClientStatus& setStatus(EClientStatus status);

	void process(Interface *pInterface) override;

private:
	struct : Packet::Data
	{
		bool bIsSelf;
		ui32 netId;
		EClientStatus status;
	} mData;

};

NS_END
NS_END
