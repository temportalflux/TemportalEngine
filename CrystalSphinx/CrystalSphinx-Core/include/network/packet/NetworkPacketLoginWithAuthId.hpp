#pragma once

#include "network/NetworkPacket.hpp"

#include "utility/Guid.hpp"

NS_NETWORK
NS_PACKET

/**
 * Sent from client to server to begin authenticating that they are the user for a given id.
 */
class LoginWithAuthId : public Packet
{
	DECLARE_PACKET_TYPE(LoginWithAuthId)

public:
	LoginWithAuthId();

	LoginWithAuthId& setId(utility::Guid const& id);

	void write(Buffer &archive) const override;
	void read(Buffer &archive) override;
	void process(Interface *pInterface) override;

private:
	utility::Guid mId;

};

NS_END
NS_END
