#include "game/GameServer.hpp"

#include "game/GameInstance.hpp"
#include "network/NetworkInterface.hpp"
#include "network/packet/NetworkPacketChatMessage.hpp"
#include "network/packet/NetworkPacketChunkReplication.hpp"
#include "network/packet/NetworkPacketUpdateUserInfo.hpp"
#include "world/WorldSaved.hpp"

using namespace game;

logging::Logger SERVER_LOG = DeclareLog("Server", LOG_INFO);

Server::Server() : Session()
{
	this->serverRSA().generate();
}

Server::~Server()
{
}

void Server::loadFrom(saveData::Instance *saveInstance)
{
	this->mServerSettings = saveData::ServerSettings(saveInstance->root());
	this->mServerSettings.readFromDisk();
	this->userRegistry().scan(saveInstance->userDirectory());
}

logging::Logger& Server::logger()
{
	return SERVER_LOG;
}

void Server::setupNetwork(utility::Flags<network::EType> flags)
{
	auto& networkInterface = *Game::networkInterface();
	networkInterface.onNetworkStarted.bind(this->weak_from_this(), std::bind(
		&Server::onNetworkStarted, this, std::placeholders::_1
	), 0);
	networkInterface.onConnectionEstablished.bind(this->weak_from_this(), std::bind(
		&Server::onClientConnected, this,
		std::placeholders::_1, std::placeholders::_2
	), 0);
	networkInterface.OnClientAuthenticatedOnServer.bind(std::bind(
		&Server::onClientAuthenticated, this,
		std::placeholders::_1, std::placeholders::_2
	));
	networkInterface.OnDedicatedClientDisconnected.bind(this->weak_from_this(), std::bind(
		&game::Server::onDedicatedClientDisconnected, this,
		std::placeholders::_1, std::placeholders::_2
	), 0);
	networkInterface.onConnectionClosed.bind(std::bind(
		&game::Server::onNetworkConnnectionClosed, this,
		std::placeholders::_1, std::placeholders::_2, std::placeholders::_3
	));
	networkInterface.onNetworkStopped.bind(this->weak_from_this(), std::bind(
		&Server::onNetworkStopped, this, std::placeholders::_1
	), 0);

	networkInterface
		.setType(flags)
		.setAddress(network::Address().setPort(this->mServerSettings.port()));
}

void Server::onNetworkStarted(network::Interface *pInterface)
{
	assert(game::Game::Get()->world());
}

void Server::onClientConnected(network::Interface *pInterface, network::ConnectionId connId)
{
}

void Server::kick(network::Identifier netId)
{
	auto* pInterface = Game::networkInterface();
	pInterface->closeConnection(pInterface->getConnectionFor(netId));
}

void Server::onClientAuthenticated(network::Interface *pInterface, network::Identifier netId)
{
	auto const connId = pInterface->getConnectionFor(netId);
	auto const userId = this->removePendingAuthentication(connId);
	this->addConnectedUser(netId);
	this->findConnectedUser(netId) = userId;

	// Tell the newly joined user about all the existing clients
	for (auto const& anyNetId : pInterface->connectedClientNetIds())
	{
		if (anyNetId == netId) continue;
		network::packet::UpdateUserInfo::create()
			->setNetId(anyNetId)
			.setInfo(this->userRegistry().loadInfo(
				this->findConnectedUser(anyNetId)
			))
			.sendTo(netId);
	}

	auto pWorld = std::dynamic_pointer_cast<world::WorldSaved>(this->world());
	assert(pWorld);
	
	// TODO: Load player location and rotation from save data
	auto const playerLocation = pWorld->makeSpawnLocation();

	// Replicate initial world data (both dedicated and integrated clients)
	network::packet::ChunkReplication::create()
		->initializeDimension(0, { playerLocation.chunk() })
		.send(connId);

	auto const entityId = game::Game::Get()->world()->createPlayer(netId, playerLocation);
	this->associatePlayer(netId, entityId);
}

void Server::onDedicatedClientDisconnected(network::Interface *pInterface, network::Identifier netId)
{
	assert(pInterface->type().includes(network::EType::eServer));
	if (this->hasConnectedUser(netId))
	{
		auto const& userId = this->findConnectedUser(netId);
		if (userId.isValid())
		{
			auto userInfo = this->userRegistry().loadInfo(userId);
			network::packet::ChatMessage::broadcastServerMessage(
				utility::formatStr("%s has left the server.", userInfo.name().c_str())
			);
			this->destroyPlayer(netId);
		}
	}
}

void Server::onNetworkConnnectionClosed(
	network::Interface *pInterface, network::ConnectionId connection,
	std::optional<network::Identifier> netId
)
{
	assert(pInterface->type().includes(network::EType::eServer));
	if (netId) this->removeConnectedUser(*netId);
}

void Server::onNetworkStopped(network::Interface *pInterface)
{
	this->clearConnectedUsers();
}

bool Server::hasSaveForUser(utility::Guid const& id) const
{
	return this->userRegistry().contains(id);
}

void Server::initializeUser(utility::Guid const& id, crypto::RSAKey const& key)
{
	auto& registry = this->userRegistry();
	registry.addId(id);
	registry.initializeUser(id, key);
}

crypto::RSAKey Server::getUserPublicKey(utility::Guid const& id) const
{
	return this->userRegistry().loadKey(id);
}

game::UserInfo Server::getUserInfo(utility::Guid const& id) const
{
	return this->userRegistry().loadInfo(id);
}

void Server::addPendingAuthentication(network::ConnectionId connId, utility::Guid const& userId)
{
	this->mPendingAuthentications.insert(std::make_pair(connId, userId));
}

utility::Guid Server::removePendingAuthentication(network::ConnectionId connId)
{
	auto iter = this->mPendingAuthentications.find(connId);
	assert(iter != this->mPendingAuthentications.end());
	auto userId = iter->second;
	this->mPendingAuthentications.erase(iter);
	return userId;
}

void Server::associatePlayer(network::Identifier netId, ecs::Identifier entityId)
{
	SERVER_LOG.log(LOG_VERBOSE, "Linking network-id %u to player entity %u", netId, entityId);
	this->mNetIdToPlayerEntityId.insert(std::make_pair(netId, entityId));
}

void Server::destroyPlayer(network::Identifier netId)
{
	auto iter = this->mNetIdToPlayerEntityId.find(netId);
	assert(iter != this->mNetIdToPlayerEntityId.end());
	SERVER_LOG.log(LOG_VERBOSE, "Unlinking network-id %u from player entity %u", netId, iter->second);
	game::Game::Get()->world()->destroyPlayer(netId, iter->second);
	this->mNetIdToPlayerEntityId.erase(iter);
}
