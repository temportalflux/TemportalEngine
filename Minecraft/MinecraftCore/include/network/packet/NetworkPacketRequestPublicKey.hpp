#pragma once

#include "network/NetworkPacket.hpp"

#include "crypto/RSA.hpp"

NS_NETWORK
NS_PACKET

/**
 * Sent from server to client and back the first time a user joins a server
 * to ensure the server has their public key.
 */
class RequestPublicKey : public Packet
{
	DECLARE_PACKET_TYPE(RequestPublicKey)

public:
	RequestPublicKey();

	void write(Buffer &archive) const override;
	void read(Buffer &archive) override;
	void process(Interface *pInterface) override;

private:
	crypto::RSAKey::PublicData mClientPublicKey;

};

NS_END
NS_END
