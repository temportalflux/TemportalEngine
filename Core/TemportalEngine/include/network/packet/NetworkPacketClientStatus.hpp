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

	void write(Buffer &archive) const override;
	void read(Buffer &archive) override;
	void process(Interface *pInterface) override;

private:
	bool mbIsSelf;
	ui32 mNetId;
	EClientStatus mStatus;

};

NS_END
NS_END
