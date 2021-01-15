#pragma once

#include "network/NetworkPacket.hpp"

#include "utility/Guid.hpp"
#include "crypto/RSA.hpp"

NS_NETWORK
NS_PACKET

class Authenticate : public Packet
{
	DECLARE_PACKET_TYPE(Authenticate)

public:
	using Token = std::vector<ui8>;

	Authenticate();

	Authenticate& setToken(Token const& token);
	Authenticate& setServerPublicKey(crypto::RSAKey::PublicData const& key);

	void write(Buffer &archive) const override;
	void read(Buffer &archive) override;
	static void sendAuthToken(ui32 connection, utility::Guid const& userId);
	void process(Interface *pInterface) override;

private:
	Token mToken;
	crypto::RSAKey::PublicData mServerPublicKey;

};

NS_END
NS_END
