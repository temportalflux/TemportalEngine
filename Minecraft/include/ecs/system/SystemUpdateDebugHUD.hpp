#pragma once

#include "ecs/system/System.hpp"

class Window;
FORWARD_DEF(NS_GRAPHICS, class UIString);
FORWARD_DEF(NS_GRAPHICS, class UIRenderer);

NS_ECS
FORWARD_DEF(NS_VIEW, class DebugHUD);

NS_SYSTEM

class UpdateDebugHUD : public System
{

public:
	UpdateDebugHUD(std::weak_ptr<Window> window);

	void createHUD(std::shared_ptr<graphics::UIRenderer> renderer);

	void update(f32 deltaTime, std::shared_ptr<ecs::view::View> view) override;
	void updateOnOccurance(std::shared_ptr<ecs::view::View> view);

private:
	std::weak_ptr<Window> mpWindow;

	struct
	{
		std::shared_ptr<graphics::UIString> transformPosition;
		std::shared_ptr<graphics::UIString> transformForward;
		std::shared_ptr<graphics::UIString> fps;
	} mUIStrings;

	ui32 mOccurance;

};

NS_END
NS_END