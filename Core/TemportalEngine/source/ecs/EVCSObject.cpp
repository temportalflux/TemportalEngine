#include "ecs/IEVCSObject.hpp"

#include "ecs/Core.hpp"
#include "network/packet/NetworkPacketECSReplicate.hpp"

using namespace ecs;

ecs::EType IEVCSObject::objectType() const { assert(false); return EType::eSystem; }
ecs::TypeId IEVCSObject::typeId() const { assert(false); return 0; }

void IEVCSObject::setOwner(std::optional<ui32> ownerNetId)
{
	this->bHasOwner = ownerNetId.has_value();
	this->ownerNetId = ownerNetId ? *ownerNetId : 0;

	auto* ecs = ecs::Core::Get();

	ecs::Core::logger().log(
		LOG_VERBOSE, "Setting owner of %s id(%u) net-id(%u) to %s",
		ecs->fullTypeName(this->objectType(), this->typeId()).c_str(),
		this->id, this->netId,
		(this->bHasOwner ? "client " + std::to_string(this->ownerNetId) : "server").c_str()
	);

	if (ecs->shouldReplicate())
	{
		ecs->replicateUpdate(this->objectType(), this->typeId(), this->netId)
			->setOwner(ownerNetId);
	}

	ecs->onOwnershipChanged.broadcast(this->objectType(), this->typeId(), this);
}

std::optional<ui32> IEVCSObject::owner() const
{
	return this->bHasOwner ? std::make_optional(this->ownerNetId) : std::nullopt;
}
