#include "game/GameSession.hpp"

#include "asset/BlockType.hpp"
#include "game/GameInstance.hpp"
#include "world/World.hpp"
#include "registry/VoxelType.hpp"

using namespace game;

Session::Session()
{
}

Session::~Session()
{
}

std::shared_ptr<game::VoxelTypeRegistry> Session::voxelTypeRegistry() { return this->mpVoxelTypeRegistry; }
std::shared_ptr<world::World> Session::world() const { return game::Game::Get()->world(); }

void Session::init()
{
	this->createVoxelTypeRegistry();
}

void Session::uninit()
{
	this->mpVoxelTypeRegistry.reset();
}

void Session::createVoxelTypeRegistry()
{
	this->mpVoxelTypeRegistry = std::make_shared<game::VoxelTypeRegistry>();

	this->logger().log(LOG_INFO, "Gathering block types...");
	auto blockList = engine::Engine::Get()->getAssetManager()->getAssetList<asset::BlockType>();
	this->logger().log(LOG_INFO, "Found %i block types", blockList.size());
	this->mpVoxelTypeRegistry->registerEntries(blockList);
}

game::UserIdRegistry& Session::userRegistry() { return this->mUserRegistry; }
game::UserIdRegistry const& Session::userRegistry() const { return this->mUserRegistry; }

std::map<ui32, utility::Guid> const& Session::connectedUsers() const { return this->mConnectedUsers; }

void Session::addConnectedUser(network::Identifier netId)
{
	this->mConnectedUsers.insert(std::make_pair(netId, utility::Guid()));
}

bool Session::hasConnectedUser(network::Identifier netId) const
{
	return this->mConnectedUsers.find(netId) != this->mConnectedUsers.end();
}

bool Session::hasConnectedUser(utility::Guid const& id) const
{
	for (auto const& [netId, userId] : this->connectedUsers())
	{
		if (id == userId) return true;
	}
	return false;
}

utility::Guid& Session::findConnectedUser(network::Identifier netId)
{
	auto iter = this->mConnectedUsers.find(netId);
	assert(iter != this->mConnectedUsers.end());
	return iter->second;
}

void Session::removeConnectedUser(network::Identifier netId)
{
	auto iter = this->mConnectedUsers.find(netId);
	assert(iter != this->mConnectedUsers.end());
	this->mConnectedUsers.erase(iter);
}

void Session::clearConnectedUsers()
{
	this->mConnectedUsers.clear();
}
