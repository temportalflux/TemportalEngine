#include "ecs/component/Component.hpp"

#include "ecs/Core.hpp"
#include "network/packet/NetworkPacketECSReplicate.hpp"

using namespace ecs::component;

ecs::EType Component::objectType() const { return EType::eComponent; }

std::shared_ptr<network::packet::ECSReplicate> Component::replicateUpdate()
{
	return ecs::Core::Get()->replicateUpdate(
		ecs::EType::eComponent, this->typeId(), this->netId
	);
}
