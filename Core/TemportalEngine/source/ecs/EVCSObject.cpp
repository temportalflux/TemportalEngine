#include "ecs/IEVCSObject.hpp"

#include "ecs/Core.hpp"
#include "network/packet/NetworkPacketECSReplicate.hpp"

using namespace ecs;

ecs::EType IEVCSObject::objectType() const { assert(false); return EType::eSystem; }
ecs::TypeId IEVCSObject::typeId() const { assert(false); return 0; }

void IEVCSObject::setId(Identifier const& id) { this->mId = id; }
Identifier const& IEVCSObject::id() const { return this->mId; }

void IEVCSObject::setNetId(Identifier const& netId)
{
	this->mbIsReplicated = true;
	this->mNetId = netId;
}

bool IEVCSObject::isReplicated() const { return this->mbIsReplicated; }

Identifier const& IEVCSObject::netId() const
{
	return this->mNetId;
}

void IEVCSObject::setOwner(std::optional<ui32> ownerNetId)
{
	this->mbHasOwner = ownerNetId.has_value();
	this->mOwnerNetId = ownerNetId ? *ownerNetId : 0;

	auto* ecs = ecs::Core::Get();

	ecs::Core::logger().log(
		LOG_VERBOSE, "Setting owner of %s id(%u)/net(%u) to %s",
		ecs->fullTypeName(this->objectType(), this->typeId()).c_str(),
		this->id(), this->netId(),
		(this->mbHasOwner ? "client net(" + std::to_string(this->mOwnerNetId) + ")" : "server").c_str()
	);

	if (ecs->shouldReplicate())
	{
		ecs->replicateUpdate(this->objectType(), this->typeId(), this->netId())
			->setOwner(ownerNetId);
	}

	ecs->onOwnershipChanged.broadcast(this->objectType(), this->typeId(), this);
}

std::optional<ui32> IEVCSObject::owner() const
{
	return this->mbHasOwner ? std::make_optional(this->mOwnerNetId) : std::nullopt;
}

void IEVCSObject::validate()
{
}
