#include "network/packet/NetworkPacketUpdateUserInfo.hpp"

#include "Engine.hpp"
#include "game/GameInstance.hpp"
#include "game/GameClient.hpp"
#include "game/GameServer.hpp"
#include "network/NetworkInterface.hpp"
#include "network/packet/NetworkPacketChatMessage.hpp"
#include "utility/Colors.hpp"

using namespace network;
using namespace network::packet;

DEFINE_PACKET_TYPE(UpdateUserInfo)

UpdateUserInfo::UpdateUserInfo()
	: Packet(EPacketFlags::eReliable)
{
}

UpdateUserInfo& UpdateUserInfo::setNetId(ui32 netId)
{
	this->mNetId = netId;
	return *this;
}

UpdateUserInfo& UpdateUserInfo::setInfo(game::UserInfo const& info)
{
	this->mName = info.name();
	this->mColorOnConnectedServer = info.color();
	return *this;
}

void UpdateUserInfo::write(Buffer &archive) const
{
	Packet::write(archive);
	if (archive.type().includes(EType::eServer))
	{
		network::write(archive, "netId", this->mNetId);
		network::write(archive, "color", this->mColorOnConnectedServer);
	}
	network::write(archive, "name", this->mName);
}

void UpdateUserInfo::read(Buffer &archive)
{
	Packet::read(archive);
	if (archive.type().includes(EType::eServer))
	{
		network::read(archive, "netId", this->mNetId);
		network::read(archive, "color", this->mColorOnConnectedServer);
	}
	network::read(archive, "name", this->mName);
}

void UpdateUserInfo::process(Interface *pInterface)
{
	auto pGame = game::Game::Get();
	if (pInterface->type().includes(EType::eServer))
	{
		// NOTE: This value is also used in the client section if running an integrated client-server
		this->mNetId = pInterface->getNetIdFor(this->connection());
		
		// NOTE: If this packet is ever received by the server
		// AFTER the first time the user sends it on log in, their color will change every time
		this->mColorOnConnectedServer = game::randColor();
	}
	network::logger().log(LOG_INFO, "Received user info for network-id %u", this->mNetId);
	if (pInterface->type().includes(EType::eServer))
	{
		auto const& userId = pGame->server()->findConnectedUser(this->mNetId);
		pGame->server()
			->getUserInfo(userId)
			.setName(this->mName)
			.setColor(this->mColorOnConnectedServer)
			.writeToDisk();
		this->broadcast({ this->connection() });

		ChatMessage::broadcastServerMessage(
			this->mName + " has joined the server."
		);
	}
	// For dedicated and integrated clients
	if (pInterface->type().includes(EType::eClient))
	{
		assert(pGame->client()->hasConnectedUser(this->mNetId));
		pGame->client()->getConnectedUserInfo(this->mNetId)
			.setName(this->mName)
			.setColor(this->mColorOnConnectedServer);
	}
}
