#pragma once

#include "ecs/component/Component.hpp"

#include "input/Event.hpp"

NS_ECS
NS_COMPONENT

class PlayerInput : public Component, public std::enable_shared_from_this<PlayerInput>
{
	DECLARE_ECS_COMPONENT_STATICS(1)

public:
	PlayerInput();
	~PlayerInput();

	void subscribeToQueue();

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

	f32 const& axialMoveSpeed() const;
	std::array<InputMapping, 6> const& axialMoveMappings() const;
	std::vector<InputAxis*> lookAxes();

private:
	f32 mAxialMoveSpeed;
	std::array<InputMapping, 6> mAxisMappings;
	InputAxis mLookHorizontal, mLookVertical;

	void onKeyInput(input::Event const & evt);
	void onMouseMove(input::Event const & evt);

};

NS_END
NS_END
