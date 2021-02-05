#pragma once

#include "ecs/component/Component.hpp"

#include "math/Matrix.hpp"

NS_EVCS
NS_COMPONENT

class PhysicsBody : public Component
{
	DECLARE_ECS_COMPONENT_STATICS(16)

public:
	PhysicsBody();

	PhysicsBody& setVelocity(math::Vector3 const& velocity);
	PhysicsBody& setAcceleration(math::Vector3 const& acceleration);

	PhysicsBody& setMass(f32 const& mass);
	f32 const& mass() const;
	f32 const& massInverse() const;

	PhysicsBody& setCenterOfMass(math::Vector3 const& offset);
	math::Vector3 const& centerOfMassOffset() const;
	
	PhysicsBody& addForce(math::Vector3 const& force);
	PhysicsBody& applyForce(math::Vector3 const& force, f32 const& deltaTime);

	PhysicsBody& addTorque(math::Vector3 const& torque, math::Vector3 const& relativeOffset);

public:

	PhysicsBody& applyTorque();

	/**
	 * Performs an Euler Kinematic integration of linear acceleration and velocity.
	 * Returns the change in position over deltaTime.
	 */
	math::Vector3 integrateLinear(f32 const& deltaTime);

	/**
	 * Performs an Euler Kinematic integration of angular acceleration and velocity.
	 */
	PhysicsBody& integrateAngular(math::Quaternion &rotation, f32 const& deltaTime);

private:
	f32 mMass, mMassInverse;
	math::Vector3 mCenterOfMassOffset;

	struct DynamicsSet
	{
		math::Vector3 velocity;
		math::Vector3 acceleration;
		math::Vector3 momentum;
		math::Vector3 force;
	};

	DynamicsSet mLinear, mAngular;

};

NS_END
NS_END
