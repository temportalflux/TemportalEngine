#include "network/packet/NetworkPacketChatMessage.hpp"

#include "Engine.hpp"
#include "game/GameClient.hpp"
#include "game/GameInstance.hpp"
#include "game/GameServer.hpp"
#include "network/NetworkInterface.hpp"
#include "ui/TextLogMenu.hpp"

using namespace network;
using namespace network::packet;

logging::Logger CHAT_LOG = DeclareLog("Chat", LOG_INFO);

DEFINE_PACKET_TYPE(ChatMessage)

ChatMessage::ChatMessage()
	: Packet(EPacketFlags::eReliable)
{
}

void ChatMessage::broadcastServerMessage(std::string const& msg)
{
	auto packet = ChatMessage::create();
	packet->mSenderNetId = 0;
	packet->mbIsServerMessage = true;
	packet->setMessage(msg);
	packet->broadcast();

	auto netType = game::Game::networkInterface()->type();
	packet->writeToLog(netType);
}

ChatMessage& ChatMessage::setMessage(std::string const& msg)
{
	this->mMsg = msg;
	return *this;
}

void ChatMessage::write(Buffer &archive) const
{
	Packet::write(archive);
	network::write(archive, "bFromServer", this->mbIsServerMessage);
	network::write(archive, "senderNetId", this->mSenderNetId);
	network::write(archive, "message", this->mMsg);
}

void ChatMessage::read(Buffer &archive)
{
	Packet::read(archive);
	network::read(archive, "bFromServer", this->mbIsServerMessage);
	network::read(archive, "senderNetId", this->mSenderNetId);
	network::read(archive, "message", this->mMsg);
}

void ChatMessage::process(network::Interface *pInterface)
{
	if (pInterface->type().includes(EType::eServer))
	{
		this->mbIsServerMessage = false;
		this->mSenderNetId = pInterface->getNetIdFor(this->connection());
		this->broadcast();
	}
	this->writeToLog(pInterface->type());
}

void ChatMessage::writeToLog(utility::Flags<EType> netType) const
{
	std::string src = "server";
	if (!this->mbIsServerMessage)
	{
		auto pGame = game::Game::Get();
		if (netType.includes(EType::eServer))
		{
			auto const& userId = pGame->server()->findConnectedUser(this->mSenderNetId);
			auto const& userInfo = pGame->server()->getUserInfo(userId);
			src = userInfo.name();
		}
		else if (netType == EType::eClient)
		{
			auto const& userInfo = pGame->client()->getConnectedUserInfo(this->mSenderNetId);
			src = userInfo.name();
		}
	}
	CHAT_LOG.log(LOG_INFO, "<%s> %s", src.c_str(), this->mMsg.c_str());
	if (netType.includes(EType::eClient))
	{
		this->writeToClientChat();
	}
}

void ChatMessage::writeToClientChat() const
{
	std::optional<ui32> senderNetId = std::nullopt;
	if (!this->mbIsServerMessage) senderNetId = this->mSenderNetId;
	game::Game::Get()->client()->chat()->onMessageReceived(senderNetId, this->mMsg);
}
