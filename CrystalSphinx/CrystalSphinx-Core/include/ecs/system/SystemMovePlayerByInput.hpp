#pragma once

#include "ecs/system/System.hpp"

#include "input/Event.hpp"

NS_ECS
NS_SYSTEM

class MovePlayerByInput : public System
{

public:
	MovePlayerByInput();

	void subscribeToQueue();

	void update(f32 deltaTime, ecs::view::View* view) override;

private:
	struct InputMapping
	{
		input::EKey key;
		math::Vector3 direction;
		bool bIsGlobal;

		bool bIsActive;
	};
	struct InputAxis
	{
		math::Vector3 axis;
		f32 radians;
		bool bApplyBefore;

		f32 delta;
	};

	f32 mAxialMoveSpeed;
	std::array<InputMapping, 6> mAxisMappings;
	InputAxis mLookHorizontal, mLookVertical;

	void onKeyInput(input::Event const & evt);
	void onMouseMove(input::Event const & evt);

};

NS_END
NS_END
