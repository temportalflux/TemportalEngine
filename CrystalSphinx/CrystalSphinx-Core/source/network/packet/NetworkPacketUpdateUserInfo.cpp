#include "network/packet/NetworkPacketUpdateUserInfo.hpp"

#include "Engine.hpp"
#include "game/GameInstance.hpp"
#include "game/GameClient.hpp"
#include "game/GameServer.hpp"
#include "network/NetworkInterface.hpp"
#include "network/packet/NetworkPacketChatMessage.hpp"

using namespace network;
using namespace network::packet;

DEFINE_PACKET_TYPE(UpdateUserInfo)

UpdateUserInfo::UpdateUserInfo()
	: Packet(EPacketFlags::eReliable)
{
}

UpdateUserInfo& UpdateUserInfo::setNetId(ui32 netId)
{
	this->mName = netId;
	return *this;
}

UpdateUserInfo& UpdateUserInfo::setInfo(game::UserInfo const& info)
{
	this->mName = info.name();
	return *this;
}

void UpdateUserInfo::write(Buffer &archive) const
{
	Packet::write(archive);
	network::write(archive, this->mNetId);
	network::write(archive, this->mName);
}

void UpdateUserInfo::read(Buffer &archive)
{
	Packet::read(archive);
	network::read(archive, this->mNetId);
	network::read(archive, this->mName);
}

void UpdateUserInfo::process(Interface *pInterface)
{
	auto pGame = game::Game::Get();
	network::logger().log(LOG_INFO, "Received alias %s for network-id %u", this->mName.c_str(), this->mNetId);
	if (pInterface->type().includes(EType::eServer))
	{
		// NOTE: This value is also used in the client section if running an integrated client-server
		this->mNetId = pInterface->getNetIdFor(this->connection());

		auto const& userId = pGame->server()->findConnectedUser(this->mNetId);
		pGame->server()->getUserInfo(userId).setName(this->mName).writeToDisk();
		this->broadcast();

		ChatMessage::broadcastServerMessage(
			this->mName + " has joined the server."
		);
	}
	// For dedicated and integrated clients
	if (pInterface->type().includes(EType::eClient))
	{
		auto& userInfo = pGame->client()->getConnectedUserInfo(this->mNetId);
		userInfo.setName(this->mName);
	}
}
