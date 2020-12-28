#pragma once

#include "CoreInclude.hpp"
#include "ui/Core.hpp"
#include "ui/ImageWidget.hpp"

NS_UI
class ImageWidgetRenderer;

class TextLogMenu
{
	
public:
	TextLogMenu();
	~TextLogMenu();

	void addImagesToRenderer(ui::ImageWidgetRenderer *renderer);

private:
	std::shared_ptr<ui::Image> mpInputBarBkgd;
	std::shared_ptr<ui::Image> mpBackgroundDemo;

};

NS_END
