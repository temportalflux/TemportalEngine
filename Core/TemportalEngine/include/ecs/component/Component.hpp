#pragma once

#include "ecs/IEVCSObject.hpp"

#define DECLARE_ECS_COMPONENT_STATICS(POOL_SIZE) \
	public: \
		static ecs::TypeId TypeId; \
		static inline constexpr uSize const MaxPoolSize = POOL_SIZE; \
		static void construct(ecs::component::Component* ptr); \
		ecs::TypeId typeId() const override;
#define DEFINE_ECS_COMPONENT_STATICS(COMP_TYPE) \
	ecs::TypeId COMP_TYPE::TypeId = 0; \
	void COMP_TYPE::construct(ecs::component::Component* ptr) { new (ptr) COMP_TYPE(); } \
	ecs::TypeId COMP_TYPE::typeId() const { return COMP_TYPE::TypeId; }

NS_NETWORK
FORWARD_DEF(NS_PACKET, class ECSReplicate);
NS_END

NS_ECS
NS_COMPONENT

class Component : public ecs::IEVCSObject
{
public:
	ecs::EType objectType() const;
	std::shared_ptr<network::packet::ECSReplicate> replicateUpdate();
};

NS_END
NS_END
