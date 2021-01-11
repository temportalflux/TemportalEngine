#include "network/NetworkPacketChatMessage.hpp"

#include "Engine.hpp"
#include "game/GameInstance.hpp"
#include "game/GameClient.hpp"
#include "network/NetworkInterface.hpp"
#include "ui/TextLogMenu.hpp"

using namespace network;
using namespace network::packet;

logging::Logger CHAT_LOG = DeclareLog("Chat");

DEFINE_PACKET_TYPE(ChatMessage, mData)

ChatMessage::ChatMessage()
	: Packet(EPacketFlags::eReliable)
{
}

ChatMessage& ChatMessage::setMessage(std::string const& msg)
{
	assert(msg.length() * sizeof(char) < sizeof(this->mData.msg));
	strcpy_s(this->mData.msg, msg.c_str());
	return *this;
}

void ChatMessage::process(network::Interface *pInterface)
{
	auto pGame = game::Game::Get();
	switch (pInterface->type())
	{
	case EType::eServer:
	{
		auto netId = pInterface->getNetIdFor(this->connection());
		this->mData.senderNetId = netId;
		CHAT_LOG.log(LOG_INFO, "%s: %s", pGame->findConnectedUser(netId).name.c_str(), this->mData.msg);
		this->broadcast();
		break;
	}
	case EType::eClient:
	{
		auto userId = pGame->findConnectedUser(this->mData.senderNetId);
		CHAT_LOG.log(LOG_INFO, "(%i)<%s> %s", this->mData.senderNetId, userId.name.c_str(), this->mData.msg);
		pGame->client()->chat()->onMessageReceived(this->mData.senderNetId, this->mData.msg);
		break;
	}
	default: break;
	}
}
