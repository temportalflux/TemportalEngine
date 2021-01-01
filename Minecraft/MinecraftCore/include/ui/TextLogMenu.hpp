#pragma once

#include "CoreInclude.hpp"
#include "ui/Core.hpp"
#include "ui/ImageWidget.hpp"
#include "ui/TextWidget.hpp"

NS_UI
class WidgetRenderer;

class TextLogMenu
{
	
public:
	TextLogMenu();
	~TextLogMenu();

	void addImagesToRenderer(ui::WidgetRenderer *renderer);
	void setIsVisible(bool bVisible);

private:
	std::shared_ptr<ui::Image> mpInputBarBkgd;
	std::shared_ptr<ui::Image> mpLogBkgd;
	std::shared_ptr<ui::Text> mpInputText;

	std::shared_ptr<ui::Image> mpBackgroundDemo;
	std::vector<std::shared_ptr<ui::Image>> mSlots;

};

NS_END
