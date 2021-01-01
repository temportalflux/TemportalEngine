#include "ui/TextLogMenu.hpp"

#include "Game.hpp"
#include "ui/UIWidgets.hpp"
#include "ui/WidgetRenderer.hpp"

using namespace ui;

TextLogMenu::TextLogMenu()
{
	(*(this->mpInputBarBkgd = std::make_shared<ui::Image>()))
		.setResource(ui::RES_IMG_WHITE)
		.setColor({ 0.f, 0.f, 0.f, 0.9f })
		.setAnchor({ 0, 1 }).setPivot({ 0, 1 })
		.setSize({ 0, 45 }).setFillWidth(true)
		;
	(*(this->mpLogBkgd = std::make_shared<ui::Image>()))
		.setResource(ui::RES_IMG_WHITE)
		.setColor({ 0.f, 0.f, 0.f, 0.9f })
		.setAnchor({ 0, 1 }).setPivot({ 0, 1 })
		.setPosition({ 0, -60 }).setSize({ 800, 500 })
		;

	/*
	(*(this->mpInputText = std::make_shared<ui::Text>()))
		.setFontOwner(game::Game::Get()->uiFontOwner())
		.setAnchor({ 0.5, 0 }).setPivot({ 0.5, 0 })
		.setFont("unispace").setFontSize(30)
		.setContent("Sphinx of Black Quartz, Judge my vow");
	//*/

	/*
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
	//*/

	this->setIsVisible(false);
}

TextLogMenu::~TextLogMenu()
{
	this->mpInputBarBkgd.reset();
	this->mpLogBkgd.reset();
	this->mpInputText.reset();

	this->mpBackgroundDemo.reset();
	this->mSlots.clear();
}

void TextLogMenu::addImagesToRenderer(ui::WidgetRenderer *renderer)
{
	renderer->add(this->mpInputBarBkgd);
	renderer->add(this->mpLogBkgd);
	//renderer->add(this->mpInputText);
	//renderer->add(this->mpBackgroundDemo);
	//for (auto& slot : this->mSlots) renderer->add(slot);
}

void TextLogMenu::setIsVisible(bool bVisible)
{
	this->mpInputBarBkgd->setIsVisible(bVisible);
	this->mpLogBkgd->setIsVisible(bVisible);
	//this->mpInputText->setIsVisible(bVisible);
}
