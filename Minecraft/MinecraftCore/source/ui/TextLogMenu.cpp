#include "ui/TextLogMenu.hpp"

#include "Engine.hpp"
#include "game/GameInstance.hpp"
#include "input/InputCore.hpp"
#include "input/Queue.hpp"
#include "ui/UIWidgets.hpp"
#include "ui/WidgetRenderer.hpp"
#include "utility/StringUtils.hpp"

using namespace ui;

logging::Logger TEXTLOGMENU_LOG = DeclareLog("TextLogMenu");

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

	(*(this->mpInputText = std::make_shared<ui::Input>()))
		.setFontOwner(game::Game::Get()->uiFontOwner())
		.setParent(this->mpInputBarBkgd)
		.setAnchor({ 0, 0.5 }).setPivot({ 0, 0.5 })
		.setFont("unispace").setFontSize(20)
		.setMaxContentLength(255);
	this->mpInputText->onConfirm.bind(std::bind(&TextLogMenu::onInputConfirmed, this, std::placeholders::_1));

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
}

TextLogMenu::~TextLogMenu()
{
	this->mpInputBarBkgd.reset();
	this->mpLogBkgd.reset();
	this->mpInputText.reset();

	this->mpBackgroundDemo.reset();
	this->mSlots.clear();
}

void TextLogMenu::init(ui::WidgetRenderer *renderer)
{
	this->setIsVisible(false);
	this->startListening(input::EInputType::KEY);

	renderer->add(this->mpInputBarBkgd);
	renderer->add(this->mpLogBkgd);
	renderer->add(this->mpInputText);
	//renderer->add(this->mpBackgroundDemo);
	//for (auto& slot : this->mSlots) renderer->add(slot);
}

bool TextLogMenu::isVisible() const { return this->mbIsVisible; }

void TextLogMenu::setIsVisible(bool bVisible)
{
	this->mbIsVisible = bVisible;
	this->mpInputBarBkgd->setIsVisible(bVisible);
	this->mpLogBkgd->setIsVisible(bVisible);
	this->mpInputText->setIsVisible(bVisible);
}

void TextLogMenu::onInput(input::Event const& evt)
{
	if (evt.inputKey.key == input::EKey::GRAVE && evt.inputKey.action == input::EAction::PRESS)
	{
		this->setIsVisible(!this->isVisible());
		this->mpInputText->setActive(this->isVisible());
	}
}

void TextLogMenu::onInputConfirmed(std::string input)
{
	this->mpInputText->clear();
	
	if (input[0] == '/')
	{
		auto args = utility::split(input.substr(1), ' ');
		auto logStr = args[0] + ":";
		for (auto iter = args.begin() + 1; iter != args.end(); ++iter)
		{
			logStr += " " + *iter + ",";
		}
		TEXTLOGMENU_LOG.log(LOG_INFO, logStr.c_str());
	}
	else
	{
		TEXTLOGMENU_LOG.log(LOG_INFO, input.c_str());
	}

}
