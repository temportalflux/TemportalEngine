#pragma once

#include "network/NetworkPacket.hpp"

#include "settings/UserInfo.hpp"

NS_NETWORK
NS_PACKET

class UpdateUserInfo : public Packet
{
	DECLARE_PACKET_TYPE(UpdateUserInfo)

public:
	UpdateUserInfo();

	UpdateUserInfo& setNetId(ui32 netId);
	UpdateUserInfo& setInfo(game::UserInfo const& info);

	void write(Buffer &archive) const override;
	void read(Buffer &archive) override;
	void process(Interface *pInterface) override;

private:
	
	// empty when sent from clients
	ui32 mNetId;

	std::string mName;

};

NS_END
NS_END
