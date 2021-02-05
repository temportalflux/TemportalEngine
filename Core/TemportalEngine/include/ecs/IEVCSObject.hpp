#pragma once

#include "ecs/types.h"

NS_ECS

class IEVCSObject
{
public:
	virtual ecs::EType objectType() const;
	virtual ecs::TypeId typeId() const;
	virtual ~IEVCSObject() {}

	template <typename T>
	bool isA() const { return this->isType(T::TypeId); }
	bool isType(TypeId const& typeId) const;

	void setId(Identifier const& id);
	Identifier const& id() const;

	void setNetId(Identifier const& netId);
	bool isReplicated() const;
	Identifier const& netId() const;

	virtual void setOwner(std::optional<ui32> ownerNetId);
	std::optional<ui32> owner() const;

	virtual void validate();
	virtual void onReplicateCreate();
	virtual void onReplicateUpdate();
	virtual void onReplicateDestroy();

private:
	// The unique-id for an instance of a given component
	Identifier mId;

	// The network identifier of the object.
	// This is unique to this object (within its manager),
	// and is consistent between all network connections.
	bool mbIsReplicated;
	Identifier mNetId;
	
	// If the object is owned by a client.
	// If false, it is owned by the server.
	bool mbHasOwner;
	// The network id of the owning client.
	// `0` if `bHasOwner` is false.
	ui32 mOwnerNetId;

};

NS_END
