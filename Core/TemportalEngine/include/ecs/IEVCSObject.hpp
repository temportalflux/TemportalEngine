#pragma once

#include "ecs/types.h"

NS_ECS

class IEVCSObject
{
public:
	virtual ecs::EType objectType() const;
	virtual ecs::TypeId typeId() const;
	virtual ~IEVCSObject() {}

	// The unique-id for an instance of a given component
	Identifier id;
	// The network identifier of the object.
	// This is unique to this object (within its manager),
	// and is consistent between all network connections.
	Identifier netId;
	// If the object is owned by a client.
	// If false, it is owned by the server.
	bool bHasOwner;
	// The network id of the owning client.
	// `0` if `bHasOwner` is false.
	ui32 ownerNetId;

	void setOwner(std::optional<ui32> ownerNetId);

};

NS_END
