#include "network/packet/NetworkPacketAuthenticate.hpp"

#include "game/GameClient.hpp"
#include "game/GameInstance.hpp"
#include "game/GameServer.hpp"
#include "network/NetworkInterface.hpp"

using namespace network;
using namespace network::packet;

DEFINE_PACKET_TYPE(Authenticate)

Authenticate::Authenticate()
	: Packet(EPacketFlags::eReliable)
{
}

Authenticate& Authenticate::setToken(Token const& token)
{
	this->mToken = token;
	return *this;
}

Authenticate& Authenticate::setServerPublicKey(crypto::RSAKey::PublicData const& key)
{
	this->mServerPublicKey = key;
	return *this;
}

void Authenticate::write(Buffer &archive) const
{
	Packet::write(archive);
	network::write(archive, "token", this->mToken);
	if (archive.type().includes(EType::eServer))
	{
		network::write(archive, "serverPublicKey", this->mServerPublicKey);
	}
}

void Authenticate::read(Buffer &archive)
{
	Packet::read(archive);
	network::read(archive, "token", this->mToken);
	if (archive.type().includes(EType::eServer))
	{
		network::read(archive, "serverPublicKey", this->mServerPublicKey);
	}
}

void Authenticate::sendAuthToken(ui32 connection, utility::Guid const& userId)
{
	auto pServer = game::Game::Get()->server();
	auto userAuthKey = pServer->getUserPublicKey(userId);

	Authenticate::create()
		->setToken(userAuthKey.encrypt(connection))
		.setServerPublicKey(pServer->serverRSA().publicData())
		.send(connection);
}

void Authenticate::process(Interface *pInterface)
{
	if (pInterface->type() == EType::eClient)
	{
		auto pClient = game::Game::Get()->client();
		
		// 1. Decrypt the provided token with our private key
		auto authKey = pClient->localUserAuthKey();
		authKey.decrypt(this->mToken);

		// TESTING ONLY - forcibly alter the token so its invalid
		//this->mToken[0]++;

		// 2. Re-encrypt token with provided public key
		auto key = crypto::RSAKey();
		crypto::RSAKey::fromPublicData(this->mServerPublicKey, key);
		key.encrypt(this->mToken);
		
		// 3. Send back to server
		this->sendToServer();
	}
	else if (pInterface->type().includes(EType::eServer))
	{
		auto pServer = game::Game::Get()->server();
		auto const& serverPrivateKey = pServer->serverRSA();
		auto const connectionId = this->connection();
		auto const netId = pInterface->getNetIdFor(connectionId);
		auto const token = serverPrivateKey.decrypt<ui32>(this->mToken);
		if (token == connectionId)
		{
			// we have confirmed the user is who they say they are
			network::logger().log(LOG_DEBUG, "Network-id %i successfully authenticated", netId);
			pInterface->markClientAuthenticated(netId);
		}
		else
		{
			network::logger().log(LOG_DEBUG, "Network-id %i failed authentication", netId);
			pServer->kick(netId);
		}
	}
}
