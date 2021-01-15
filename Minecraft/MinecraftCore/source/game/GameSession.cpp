#include "game/GameSession.hpp"

using namespace game;

Session::Session()
{
}

Session::~Session()
{
}

game::UserIdRegistry& Session::userRegistry() { return this->mUserRegistry; }
game::UserIdRegistry const& Session::userRegistry() const { return this->mUserRegistry; }

std::map<ui32, utility::Guid> const& Session::connectedUsers() const { return this->mConnectedUsers; }

void Session::addConnectedUser(ui32 netId)
{
	this->mConnectedUsers.insert(std::make_pair(netId, utility::Guid()));
}

bool Session::hasConnectedUser(ui32 netId) const
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

utility::Guid& Session::findConnectedUser(ui32 netId)
{
	auto iter = this->mConnectedUsers.find(netId);
	assert(iter != this->mConnectedUsers.end());
	return iter->second;
}

void Session::removeConnectedUser(ui32 netId)
{
	auto iter = this->mConnectedUsers.find(netId);
	assert(iter != this->mConnectedUsers.end());
	this->mConnectedUsers.erase(iter);
}

