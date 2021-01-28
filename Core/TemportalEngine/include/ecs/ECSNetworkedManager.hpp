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

	virtual std::string typeName(TypeId const& typeId) const = 0;
	virtual uSize typeSize(TypeId const& typeId) const = 0;
	bool hasNetworkId(Identifier const& netId) const;
	Identifier getNetworkedObjectId(Identifier netId) const;
	virtual IEVCSObject* createObject(TypeId const& typeId, Identifier const& netId) = 0;
	virtual IEVCSObject* getObject(TypeId const& typeId, Identifier const& netId) = 0;
	virtual void destroyObject(TypeId const& typeId, Identifier const& netId) = 0;

protected:
	Identifier nextNetworkId();
	void assignNetworkId(Identifier netId, Identifier objectId);
	void removeNetworkId(Identifier netId);

private:
	std::map<Identifier, Identifier> mNetIdToObjectId;
	std::set<Identifier> mUnusedNetIds;

};

NS_END
