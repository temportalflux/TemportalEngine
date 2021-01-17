#include "ecs/ECSNetworkedManager.hpp"

using namespace ecs;

NetworkedManager::NetworkedManager()
{
}

NetworkedManager::~NetworkedManager()
{
}

void NetworkedManager::assignNetworkId(Identifier netId, Identifier objectId)
{
	this->mNetIdToObjectId.insert(std::make_pair(netId, objectId));
}

Identifier NetworkedManager::getNetworkedObjectId(Identifier netId) const
{
	auto iter = this->mNetIdToObjectId.find(netId);
	assert(iter != this->mNetIdToObjectId.end());
	return iter->second;
}

void NetworkedManager::removeNetworkId(Identifier netId)
{
	auto iter = this->mNetIdToObjectId.find(netId);
	assert(iter != this->mNetIdToObjectId.end());
	this->mNetIdToObjectId.erase(iter);
}
