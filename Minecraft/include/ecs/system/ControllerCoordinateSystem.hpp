#pragma once

#include "ITickable.hpp"

#include "input/Event.hpp"

FORWARD_DEF(NS_ECS, class CoordinateTransform)

NS_ECS

// TODO: This should be an ECS system which changes CoordinateTransform based on user input
class ControllerCoordinateSystem : public ITickable
{

public:
	ControllerCoordinateSystem();
	
	// TODO: Getting user input should be its own system
	void subscribeToInput();
	void assignCameraTransform(ecs::CoordinateTransform *transform);

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

	ecs::CoordinateTransform *mpCameraTransform;

};

NS_END
