#include "network/packet/NetworkPacketRequestPublicKey.hpp"

#include "game/GameClient.hpp"
#include "game/GameInstance.hpp"
#include "game/GameServer.hpp"
#include "network/NetworkInterface.hpp"

using namespace network;
using namespace network::packet;

DEFINE_PACKET_TYPE(RequestPublicKey)

RequestPublicKey::RequestPublicKey()
	: Packet(EPacketFlags::eReliable)
{
}

void RequestPublicKey::write(Buffer &archive) const
{
	Packet::write(archive);
	if (archive.type() == EType::eClient)
	{
		archive.writeRaw(this->mClientPublicKey);
	}
}

void RequestPublicKey::read(Buffer &archive)
{
	Packet::read(archive);
	if (archive.type() == EType::eClient)
	{
		archive.readRaw(this->mClientPublicKey);
	}
}

void RequestPublicKey::process(Interface *pInterface)
{
	if (pInterface->type() == EType::eClient)
	{
		auto const authKey = game::Game::Get()->client()->localUserAuthKey();
		this->mClientPublicKey = authKey.publicData();
		this->sendToServer();
	}
	else if (pInterface->type().includes(EType::eServer))
	{
		auto const& netId = pInterface->getNetIdFor(this->connection());
		
		auto key = crypto::RSAKey();
		crypto::RSAKey::fromPublicData(this->mClientPublicKey, key);
		
		auto pServer = game::Game::Get()->server();
		auto const& id = pServer->findConnectedUser(netId);
		pServer->initializeUser(id, key);

		pInterface->markClientAuthenticated(netId);
	}
}
