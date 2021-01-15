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
	if (pInterface->type().includes(EType::eServer))
	{
		this->mNetId = pInterface->getNetIdFor(this->connection());
		network::logger().log(LOG_INFO, "Received alias %s for network-id %u", this->mName.c_str(), this->mNetId);

		auto const& userId = pGame->server()->findConnectedUser(this->mNetId);
		auto userInfo = pGame->server()->getUserInfo(userId);
		std::string oldName = userInfo.name();
		userInfo.setName(this->mName).writeToDisk();
		this->broadcast();

		/* TODO
		ChatMessage::broadcastServerMessage(
			oldName.length() == 0
			? utility::formatStr("%s has joined the server.", this->mData.name)
			: utility::formatStr("%s is now named %s", oldName.c_str(), this->mData.name)
		);
		//*/
	}
	else if (pInterface->type() == EType::eClient)
	{
		network::logger().log(LOG_INFO, "Received alias %s for network-id %u", this->mName, this->mNetId);
		auto& userInfo = pGame->client()->getConnectedUserInfo(this->mNetId);
		userInfo.setName(this->mName);
	}
}
