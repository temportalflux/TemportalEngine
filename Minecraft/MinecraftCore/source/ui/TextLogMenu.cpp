#include "ui/TextLogMenu.hpp"

#include "ui/UIWidgets.hpp"
#include "ui/WidgetRenderer.hpp"

using namespace ui;

TextLogMenu::TextLogMenu()
{
	/*
	(*(this->mpInputBarBkgd = std::make_shared<ui::Image>()))
		.setPosition({ 0, 0 })
		.setSize({ 512, 512 })
		.setZLayer(1)
		.setTextureSize({ 1, 1 })
		.setTextureSubsize({ 1, 1 })
		.setTexturePixels({ 255, 255, 255, 255 })
		.setColor({ 0.f, 0.f, 0.f, 0.9f })
		;
	//*/

	this->mpBackgroundDemo = std::make_shared<ui::Image>();
	ui::createMenuBackground(*this->mpBackgroundDemo.get(), 30)
		.setAnchor({ 1, 1 })
		.setPivot({ 1, 1 })
		.setSize({ 512, 512 });

	ui::createSlotImage(*this->mSlots.emplace_back(std::make_shared<ui::Image>()).get(), 6)
		.setAnchorParent(this->mpBackgroundDemo)
		.setAnchor({ 0, 0.5 })
		.setPivot({ 0, 0.5 })
		.setSize({ 64, 64 });

}

TextLogMenu::~TextLogMenu()
{
	this->mpBackgroundDemo.reset();
	this->mpInputBarBkgd.reset();
	this->mSlots.clear();
}

void TextLogMenu::addImagesToRenderer(ui::WidgetRenderer *renderer)
{
	renderer->add(this->mpBackgroundDemo);
	//renderer->add(this->mpInputBarBkgd);
	for (auto& slot : this->mSlots) renderer->add(slot);
}
