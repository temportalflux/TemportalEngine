#include "ui/TextLogMenu.hpp"

#include "ui/Widgets.hpp"
#include "ui/WidgetRenderer.hpp"

using namespace ui;

TextLogMenu::TextLogMenu()
{
	(this->mpInputBarBkgd = std::make_shared<ui::Image>())
		->setTextureSize({ 1, 1 })
		.setTextureSubsize({ 1, 1 })
		.setTexturePixels({ 255, 255, 255, 255 })
		.setRenderPosition({ 0, 0 })
		.setRenderSize({ 512, 512 });

	this->mpBackgroundDemo = std::make_shared<ui::Image>();
	ui::createMenuBackground(*this->mpBackgroundDemo.get(), 30)
		.setRenderPosition({ -256, -256 })
		.setRenderSize({ 512, 512 });
}

TextLogMenu::~TextLogMenu()
{
	this->mpBackgroundDemo.reset();
	this->mpInputBarBkgd.reset();
}

void TextLogMenu::addImagesToRenderer(ui::ImageWidgetRenderer *renderer)
{
	renderer->add(this->mpBackgroundDemo);
	renderer->add(this->mpInputBarBkgd);
}
