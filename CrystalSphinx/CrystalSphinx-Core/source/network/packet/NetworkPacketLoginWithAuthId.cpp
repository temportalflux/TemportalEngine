#include "network/packet/NetworkPacketLoginWithAuthId.hpp"

#include "game/GameInstance.hpp"
#include "game/GameServer.hpp"
#include "network/NetworkInterface.hpp"
#include "network/packet/NetworkPacketAuthenticate.hpp"

using namespace network;
using namespace network::packet;

DEFINE_PACKET_TYPE(LoginWithAuthId)

LoginWithAuthId::LoginWithAuthId()
	: Packet(EPacketFlags::eReliable)
{
}

LoginWithAuthId& LoginWithAuthId::setId(utility::Guid const& id)
{
	this->mId = id;
	return *this;
}

LoginWithAuthId& LoginWithAuthId::setPublicKey(crypto::RSAKey::PublicData const& publicKey)
{
	this->mClientPublicKey = publicKey;
	return *this;
}

void LoginWithAuthId::write(Buffer &archive) const
{
	Packet::write(archive);
	network::write(archive, "id", this->mId);
	network::write(archive, "clientPublicKey", this->mClientPublicKey);
}

void LoginWithAuthId::read(Buffer &archive)
{
	Packet::read(archive);
	network::read(archive, "id", this->mId);
	network::read(archive, "clientPublicKey", this->mClientPublicKey);
}

void LoginWithAuthId::process(Interface *pInterface)
{
	if (pInterface->type().includes(EType::eServer))
	{
		auto pServer = game::Game::Get()->server();
		//auto const& netId = pInterface->getNetIdFor(this->connection());

		network::logger().log(
			LOG_INFO, "Received userId(%s) from connectionId(%u)",
			this->mId.toString().c_str(), this->connection()
		);

		if (pServer->hasConnectedUser(this->mId))
		{
			network::logger().log(
				LOG_INFO, "User with userId(%s) is already logged in. Kicking connectionId(%u).",
				this->mId.toString().c_str(), this->connection()
			);
			assert(!pInterface->type().includes(EType::eClient));
			pInterface->closeConnection(this->connection());
			return;
		}

		pServer->addPendingAuthentication(this->connection(), this->mId);

		// If the user has previously logged in, authenticate their credentials
		if (pServer->hasSaveForUser(this->mId))
		{
			Authenticate::sendAuthToken(this->connection(), this->mId);
		}
		// If the user is new, then automatically authenticate them (first come first server)
		else
		{
			// assumes that this assigns the connection a net id
			pInterface->markClientAuthenticated(this->connection());

			auto pServer = game::Game::Get()->server();
			auto key = crypto::RSAKey();
			crypto::RSAKey::fromPublicData(this->mClientPublicKey, key);
			pServer->initializeUser(this->mId, key);
		}
	}
}
