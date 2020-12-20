#pragma once

#include "physics/PhysicsObject.hpp"
#include "physics/PhysicsController.hpp"

NS_PHYSICS
class RigidBody;

class Scene : public physics::Object
{

public:
	Scene();
	~Scene();
	
	void create() override;

	Scene& setGravity(math::Vector3 const& gravity);
	math::Vector3 const& gravity() const;
	Scene& setSimulationStep(f32 const& frequency);

	Scene& addActor(RigidBody *pBody);

	void createController(Controller &controller);
	ui32 getControllerCount() const;
	void getController(ui32 const& index, Controller &out) const;

	void simulate(f32 const& deltaTime);

private:
	math::Vector3 mGravity;
	
	/*physx::PxScene*/ void* mpInternal;

	f32 mSimulationFrequency;
	f32 mTimeSinceLastSimulate;

	/*physx::PxControllerManager*/ void* mpControllerManager;

};

NS_END
