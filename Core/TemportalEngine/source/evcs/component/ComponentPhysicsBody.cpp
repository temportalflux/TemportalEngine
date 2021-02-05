#include "evcs/component/ComponentPhysicsBody.hpp"

#include "math/transform.hpp"

using namespace evcs;
using namespace evcs::component;

DEFINE_ECS_COMPONENT_STATICS(PhysicsBody)

PhysicsBody::PhysicsBody() : Component()
	, mMass(1), mMassInverse(1)
	, mCenterOfMassOffset({ 0, 0, 0 })
	, mLinear({}), mAngular({})
{
}

PhysicsBody& PhysicsBody::setVelocity(math::Vector3 const& velocity)
{
	this->mLinear.velocity = velocity;
	return *this;
}

PhysicsBody& PhysicsBody::setAcceleration(math::Vector3 const& acceleration)
{
	this->mLinear.acceleration = acceleration;
	return *this;
}

PhysicsBody& PhysicsBody::setMass(f32 const& mass)
{
	this->mMass = mass;
	this->mMassInverse = this->mMass == 0.0f ? 0.0f : (1.0f / this->mMass);
	return *this;
}

f32 const& PhysicsBody::mass() const { return this->mMass; }
f32 const& PhysicsBody::massInverse() const { return this->mMassInverse; }

PhysicsBody& PhysicsBody::setCenterOfMass(math::Vector3 const& offset)
{
	this->mCenterOfMassOffset = offset;
	return *this;
}

math::Vector3 const& PhysicsBody::centerOfMassOffset() const { return this->mCenterOfMassOffset; }

PhysicsBody& PhysicsBody::addForce(math::Vector3 const& force)
{
	this->mLinear.force += force;
	return *this;
}

PhysicsBody& PhysicsBody::addTorque(math::Vector3 const& torque, math::Vector3 const& relativeOffset)
{
	// use cross product to convert force to torque
	// torque += (offset from center of mass) x force
	this->mAngular.force += math::Vector3::cross(relativeOffset, torque);
	return *this;
}

PhysicsBody& PhysicsBody::applyForce(math::Vector3 const& force, f32 const& deltaTime)
{
	// F=ma => a = F/m => a = F*massInv
	this->mLinear.acceleration += force * this->mMassInverse * deltaTime;
	return *this;
}

PhysicsBody& PhysicsBody::applyTorque()
{
	// angularAcceleration = inertiaTensorInv * torque
	// TODO: STUB
	return *this;
}

math::Vector3 PhysicsBody::integrateLinear(f32 const& deltaTime)
{
	math::Vector3 deltaPosition = math::Vector3::ZERO;
	deltaPosition += this->mLinear.velocity * deltaTime;
	deltaPosition += 0.5f * this->mLinear.acceleration * deltaTime * deltaTime;
	this->mLinear.velocity += this->mLinear.acceleration * deltaTime;
	return deltaPosition;
}

math::Quaternion const MultiplyVector(math::Vector3 const& vector, math::Quaternion const& quat)
{
	// r = quat vector3
	// q* = < vxr + wv, -dot(v, r) >
	return math::Quaternion(
		math::Vector3::cross(vector, quat.createSubvector<3>())
		+ vector * quat.w()
	) + math::Quaternion::Identity * -math::Vector3::dot(vector, quat.createSubvector<3>());
}

PhysicsBody& PhysicsBody::integrateAngular(math::Quaternion &rotation, f32 const& deltaTime)
{
	// qPrime = q + (dq/dt)dt + (1/2)(d^2q/dt^2)dt^2
	// qPrime = q + [(1/2)wdt * q] + [(1/2)aq - (1/4)|w|^2*q]dt^2
	math::Quaternion integrated = rotation;
	{
		// integrate velocity
		integrated += MultiplyVector(0.5f * this->mAngular.velocity * deltaTime, rotation);
		
		// integrate acceleration
		auto a = MultiplyVector(0.5f * this->mAngular.acceleration, rotation);
		auto b = (0.25f * this->mAngular.velocity.magnitudeSq() * rotation);
		integrated += (0.5f * deltaTime * deltaTime) * (a - b);

		integrated.normalize();
	}
	rotation = integrated;

	this->mAngular.velocity += this->mAngular.acceleration * deltaTime;

	return *this;
}
