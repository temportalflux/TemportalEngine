#pragma once

#include "ITickable.hpp"

#include "input/Event.hpp"

#include <glm/glm.hpp>

class Camera;

class Controller : public ITickable
{

public:
	Controller();

	void subscribeToInput();
	void assignCamera(std::shared_ptr<Camera> camera);

private:
	struct InputMapping
	{
		glm::vec3 direction;
		f32 speed;
		bool bIsActive;
	};
	struct InputAxis
	{
		glm::vec3 axis; // turning axis
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
	void processInput(input::Event const & evt);
	void tick(f32 deltaTime) override;

	// TODO: Make this a generic "entity" with a transform
	std::shared_ptr<Camera> mCamera;

};
