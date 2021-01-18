#pragma once

#include "ecs/component/Component.hpp"

NS_ECS
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

	void validate() override;

private:
	bool mbAffectedByGravity;
	math::Vector3 mCollisionExtents;

};

NS_END
NS_END