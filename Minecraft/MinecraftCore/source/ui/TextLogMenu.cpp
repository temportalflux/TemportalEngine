#include "ui/TextLogMenu.hpp"

#include "Engine.hpp"
#include "game/GameInstance.hpp"
#include "input/InputCore.hpp"
#include "input/Queue.hpp"
#include "ui/UIWidgets.hpp"
#include "ui/WidgetRenderer.hpp"

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

	(*(this->mpInputText = std::make_shared<ui::Text>()))
		.setFontOwner(game::Game::Get()->uiFontOwner())
		.setParent(this->mpInputBarBkgd)
		.setAnchor({ 0, 0.5 }).setPivot({ 0, 0.5 })
		.setFont("unispace").setFontSize(20)
		.setMaxContentLength(255);

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
	
	if (bVisible) input::startTextInput();
	else input::stopTextInput();
}

void TextLogMenu::bindInput(std::shared_ptr<input::Queue> pInput)
{
	pInput->OnInputEvent.bind(
		input::EInputType::KEY, this->weak_from_this(),
		std::bind(&TextLogMenu::onInputKey, this, std::placeholders::_1)
	);
	pInput->OnInputEvent.bind(
		input::EInputType::TEXT, this->weak_from_this(),
		std::bind(&TextLogMenu::onInputText, this, std::placeholders::_1)
	);
}

void TextLogMenu::unbindInput(std::shared_ptr<input::Queue> pInput)
{
	pInput->OnInputEvent.unbind(input::EInputType::KEY, this->weak_from_this());
	pInput->OnInputEvent.unbind(input::EInputType::TEXT, this->weak_from_this());
}

void TextLogMenu::onInputKey(input::Event const& evt)
{
	auto bIsPressed = evt.inputKey.action == input::EAction::PRESS;
	auto bIsRepeat = evt.inputKey.action == input::EAction::REPEAT;
	if (evt.inputKey.key == input::EKey::GRAVE && bIsPressed)
	{
		this->setIsVisible(!this->isVisible());
		this->mInputCursorPos = 0;
		this->mInputContent = "";
		this->mpInputText->setContent("");
		return;
	}
	if (!this->isVisible()) return;
	if (bIsPressed || bIsRepeat)
	{
		if (evt.inputKey.key == input::EKey::BACKSPACE)
		{
			this->removeInputAtCursor();
			return;
		}
		if (evt.inputKey.key == input::EKey::SP_DELETE)
		{
			this->removeInputAfterCursor();
			return;
		}
	}
}

void TextLogMenu::onInputText(input::Event const& evt)
{
	if (!this->isVisible()) return;
	this->mInputContent.insert(this->mInputContent.begin() + this->mInputCursorPos, evt.inputText.text[0]);
	this->mInputCursorPos++;
	this->mpInputText->setContent(this->mInputContent);
}

void TextLogMenu::removeInputAtCursor()
{
	if (this->mInputContent.length() > 0)
	{
		this->mInputContent.erase(this->mInputContent.begin() + this->mInputCursorPos);
		if (this->mInputCursorPos > 0) this->mInputCursorPos--;
		this->mpInputText->setContent(this->mInputContent);
	}
}

void TextLogMenu::removeInputAfterCursor()
{

}
