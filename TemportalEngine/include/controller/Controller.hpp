#pragma once

#include "ITickable.hpp"

#include "input/Event.hpp"

#include <glm/glm.hpp>

class Controller : public ITickable
{

public:
	Controller();

	void subscribeToInput();

private:
	struct InputMapping
	{
		glm::vec3 direction;
		f32 speed;
		bool bIsActive;
	};
	InputMapping mForward, mBackward;
	InputMapping mStrafeLeft, mStrafeRight;
	InputMapping mUp, mDown;
	std::unordered_map<input::EKey, InputMapping*> mInputMappings;

	void onKeyInput(input::Event const & evt);
	void processInput(input::Event const & evt);
	void tick(f32 deltaTime) override;

	// TODO: modify another transform instead
	glm::vec3 mPos;

};
