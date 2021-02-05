#pragma once

#include "evcs/component/Component.hpp"

NS_EVCS
NS_COMPONENT

class PlayerPhysics : public Component
{
	DECLARE_ECS_COMPONENT_STATICS(16)

public:
	PlayerPhysics();
	PlayerPhysics(PlayerPhysics const& other) = delete;
	std::vector<Field> allFields() const;
	~PlayerPhysics();

	PlayerPhysics& setIsAffectedByGravity(bool bAffectedByGravity);
	bool isAffectedByGravity() const;

	math::Vector3 const& collisionExtents() const;

	void validate() override;

private:
	bool mbAffectedByGravity;
	math::Vector3 mCollisionExtents;

};

NS_END
NS_END
