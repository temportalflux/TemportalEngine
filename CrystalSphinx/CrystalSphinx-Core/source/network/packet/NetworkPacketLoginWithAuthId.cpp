#include "network/packet/NetworkPacketLoginWithAuthId.hpp"

#include "game/GameInstance.hpp"
#include "game/GameServer.hpp"
#include "network/NetworkInterface.hpp"
#include "network/packet/NetworkPacketRequestPublicKey.hpp"
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

void LoginWithAuthId::write(Buffer &archive) const
{
	Packet::write(archive);
	network::write(archive, "id", this->mId);
}

void LoginWithAuthId::read(Buffer &archive)
{
	Packet::read(archive);
	network::read(archive, "id", this->mId);
}

void LoginWithAuthId::process(Interface *pInterface)
{
	if (pInterface->type().includes(EType::eServer))
	{
		auto pServer = game::Game::Get()->server();
		auto const& netId = pInterface->getNetIdFor(this->connection());

		network::logger().log(LOG_INFO, "Received user id %s from NetworkUser %u", this->mId.toString().c_str(), netId);

		if (pServer->hasConnectedUser(this->mId))
		{
			network::logger().log(
				LOG_INFO, "User with userId %s is already logged in. Kicking NetworkUser %u.",
				this->mId.toString().c_str(), netId
			);
			pServer->kick(netId);
			return;
		}

		// Store the user's id with this network id.
		// If the user fails authentication,
		// they will be disconnected/kicked and the entry will be cleaned up
		pServer->findConnectedUser(netId) = this->mId;

		// If the user has previously logged in, authenticate their credentials
		if (pServer->hasSaveForUser(this->mId))
		{
			Authenticate::sendAuthToken(this->connection(), this->mId);
		}
		// If the user is new, then automatically authenticate them (first come first server)
		else
		{
			RequestPublicKey::create()->send(this->connection());
		}
	}
}
