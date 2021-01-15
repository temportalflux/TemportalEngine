#pragma once

#include "CoreInclude.hpp"
#include "ITickable.hpp"
#include "ui/Core.hpp"
#include "ui/ImageWidget.hpp"
#include "ui/TextWidget.hpp"

NS_UI
class WidgetRenderer;

class DebugHUD : public ITickable
{

public:
	DebugHUD();
	~DebugHUD();

	void addWidgetsToRenderer(ui::WidgetRenderer *renderer);
	void setIsVisible(bool bVisible);

	void tick(f32 deltaTime) override;

private:
	std::shared_ptr<ui::Text> mpAlphabet;
	std::shared_ptr<ui::Text> mpPosition;
	std::shared_ptr<ui::Text> mpFPS;

	ui32 mOccurance;

};

NS_END
