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
	packet->broadcast();

	packet->writeToLog();
	if (game::Game::networkInterface()->type().includes(EType::eClient))
	{
		packet->writeToClientChat();
	}
}

ChatMessage& ChatMessage::setMessage(std::string const& msg)
{
	assert(msg.length() * sizeof(char) < sizeof(this->mData.msg));
	strcpy_s(this->mData.msg, msg.c_str());
	return *this;
}

void ChatMessage::process(network::Interface *pInterface)
{
	if (pInterface->type().includes(EType::eServer))
	{
		this->mData.bIsServerMessage = false;
		this->mData.senderNetId = pInterface->getNetIdFor(this->connection());
		this->broadcast();
	}
	this->writeToLog();
	if (pInterface->type().includes(EType::eClient))
	{
		this->writeToClientChat();
	}
}

void ChatMessage::writeToLog() const
{
	std::string src = "server";
	if (!this->mData.bIsServerMessage)
	{
		auto pGame = game::Game::Get();
		auto userId = pGame->findConnectedUser(this->mData.senderNetId);
		src = userId.name;
	}
	CHAT_LOG.log(LOG_INFO, "<%s> %s", src.c_str(), this->mData.msg);
}

void ChatMessage::writeToClientChat() const
{
	game::Game::Get()->client()->chat()->onMessageReceived(
		!this->mData.bIsServerMessage ? std::make_optional(this->mData.senderNetId) : std::nullopt,
		this->mData.msg
	);
}
