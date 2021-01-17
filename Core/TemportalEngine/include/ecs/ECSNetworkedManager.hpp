#pragma once

#include "TemportalEnginePCH.hpp"
#include "ecs/types.h"

NS_ECS

class NetworkedManager
{

public:
	NetworkedManager();
	virtual ~NetworkedManager();

	//virtual std::shared_ptr<IEVCSObject> create(TypeId typeId) = 0;

	void assignNetworkId(Identifier netId, Identifier objectId);
	Identifier getNetworkedObjectId(Identifier netId) const;
	void removeNetworkId(Identifier netId);

private:
	std::map<Identifier, Identifier> mNetIdToObjectId;

};

NS_END
