#pragma once

#include "ITickable.hpp"

#include "input/Event.hpp"

#include <glm/glm.hpp>

FORWARD_DEF(NS_ECS, struct ComponentTransform)

class Controller : public ITickable
{

public:
	Controller();

	void subscribeToInput();
	// TODO: This should be an ecs entity or component id
	void assignCameraTransform(ecs::ComponentTransform *transform);

private:
	struct InputMapping
	{
		math::Vector3 direction;
		bool bIsGlobal;
		f32 speed;
		bool bIsActive;
	};
	struct InputAxis
	{
		math::Vector3 axis;
		f32 radians;
		f32 delta;
	};
	InputMapping mForward, mBackward;
	InputMapping mStrafeLeft, mStrafeRight;
	InputMapping mUp, mDown;
	InputAxis mLookHorizontal, mLookVertical;
	std::unordered_map<input::EKey, InputMapping*> mInputMappings;

	void onKeyInput(input::Event const & evt);
	void onMouseMove(input::Event const & evt);
	void tick(f32 deltaTime) override;

	ecs::ComponentTransform *mpCameraTransform;

};
