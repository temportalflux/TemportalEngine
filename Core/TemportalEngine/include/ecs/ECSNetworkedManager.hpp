#pragma once

#include "TemportalEnginePCH.hpp"
#include "ecs/types.h"
#include "ecs/IEVCSObject.hpp"

NS_ECS

class NetworkedManager
{

public:
	NetworkedManager();
	virtual ~NetworkedManager();

	virtual IEVCSObject* createObject(TypeId const& typeId) = 0;

	void assignNetworkId(Identifier netId, Identifier objectId);
	Identifier getNetworkedObjectId(Identifier netId) const;
	void removeNetworkId(Identifier netId);

	Identifier nextNetworkId();

private:
	std::map<Identifier, Identifier> mNetIdToObjectId;
	std::set<Identifier> mUnusedNetIds;

};

NS_END
