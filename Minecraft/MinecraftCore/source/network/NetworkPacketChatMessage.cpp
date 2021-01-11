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

void ChatMessage::broadcastServerMessage(std::string const& msg)
{
	auto packet = ChatMessage::create();
	packet->mData.senderNetId = 0;
	packet->mData.bIsServerMessage = true;
	packet->setMessage(msg);
	CHAT_LOG.log(LOG_INFO, "<%s> %s", "server", msg.c_str());
	packet->broadcast();
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
		this->mData.bIsServerMessage = false;
		this->mData.senderNetId = netId;
		auto userId = pGame->findConnectedUser(netId);
		CHAT_LOG.log(LOG_INFO, "<%s> %s", userId.name.c_str(), this->mData.msg);
		this->broadcast();
		break;
	}
	case EType::eClient:
	{
		std::optional<ui32> senderNetId = std::nullopt;
		if (this->mData.bIsServerMessage)
		{
			CHAT_LOG.log(LOG_INFO, "<%s> %s", "server", this->mData.msg);
		}
		else
		{
			senderNetId = this->mData.senderNetId;
			auto userId = pGame->findConnectedUser(this->mData.senderNetId);
			CHAT_LOG.log(LOG_INFO, "<%s> %s", userId.name.c_str(), this->mData.msg);
		}
		pGame->client()->chat()->onMessageReceived(senderNetId, this->mData.msg);
		break;
	}
	default: break;
	}
}
