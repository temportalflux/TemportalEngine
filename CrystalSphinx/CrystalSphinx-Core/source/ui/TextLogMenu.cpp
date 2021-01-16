#include "ui/TextLogMenu.hpp"

#include "Engine.hpp"
#include "command/CommandRegistry.hpp"
#include "game/GameClient.hpp"
#include "game/GameInstance.hpp"
#include "input/InputCore.hpp"
#include "input/Queue.hpp"
#include "network/packet/NetworkPacketChatMessage.hpp"
#include "ui/UIWidgets.hpp"
#include "ui/WidgetRenderer.hpp"
#include "utility/StringUtils.hpp"

using namespace ui;

logging::Logger TEXTLOGMENU_LOG = DeclareLog("TextLogMenu", LOG_INFO);

TextLogMenu::TextLogMenu()
{
	(*(this->mpInputBarBkgd = std::make_shared<ui::Image>()))
		.setResource(ui::RES_IMG_WHITE)
		.setColor({ 0.f, 0.f, 0.f, 0.9f })
		.setAnchor({ 0, 1 }).setPivot({ 0, 1 })
		.setSize({ 0, 45 }).setFillWidth(true)
		;

	(*(this->mpInputText = std::make_shared<ui::Input>()))
		.setFontOwner(game::Game::Get()->client()->uiFontOwner())
		.setParent(this->mpInputBarBkgd)
		.setAnchor({ 0, 0.5 }).setPivot({ 0, 0.5 })
		.setFont("unispace").setFontSize(20)
		.setMaxContentLength(255);
	this->mpInputText->onConfirm.bind(std::bind(&TextLogMenu::onInputConfirmed, this, std::placeholders::_1));

	(*(this->mpLogBkgd = std::make_shared<ui::Image>()))
		.setResource(ui::RES_IMG_WHITE)
		.setColor({ 0.f, 0.f, 0.f, 0.9f })
		.setAnchor({ 0, 1 }).setPivot({ 0, 1 })
		.setPosition({ 0, -60 }).setSize({ 800, 500 })
		;

	this->mDisplayableMessageCount = 10;
	(*(this->mpChatLog = std::make_shared<ui::Text>()))
		.setFontOwner(game::Game::Get()->client()->uiFontOwner())
		.setParent(this->mpLogBkgd)
		.setParentFlag(Widget::EParentFlags::eVisibility, false)
		.setFillWidth(true)
		.setAnchor({ 0, 1 }).setPivot({ 0, 1 })
		.setFont("unispace").setFontSize(20)
		.setIsVisible(true);

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
	this->mpInputText.reset();
	this->mpLogBkgd.reset();
	this->mpChatLog.reset();

	this->mpBackgroundDemo.reset();
	this->mSlots.clear();
}

void TextLogMenu::init(ui::WidgetRenderer *renderer)
{
	this->setIsVisible(false);
	this->startListening(input::EInputType::KEY);

	renderer->add(this->mpInputBarBkgd);
	renderer->add(this->mpInputText);
	renderer->add(this->mpLogBkgd);
	renderer->add(this->mpChatLog);
	//renderer->add(this->mpBackgroundDemo);
	//for (auto& slot : this->mSlots) renderer->add(slot);
}

bool TextLogMenu::isVisible() const { return this->mbIsVisible; }

void TextLogMenu::setIsVisible(bool bVisible)
{
	this->mbIsVisible = bVisible;
	this->mpInputBarBkgd->setIsVisible(bVisible);
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
		if (auto errors = engine::Engine::Get()->commands()->execute(utility::split(input.substr(1), ' ')))
		{
			for (auto const& error : errors.value()) TEXTLOGMENU_LOG.log(LOG_INFO, error.c_str());
		}
	}
	else
	{
		network::packet::ChatMessage::create()->setMessage(input).sendToServer();
	}
}

void TextLogMenu::onMessageReceived(std::optional<ui32> senderNetId, std::string const& message)
{
	std::optional<std::string> name = std::nullopt;
	if (senderNetId)
	{
		auto const& userInfo = game::Game::Get()->client()->getConnectedUserInfo(*senderNetId);
		name = userInfo.name();
	}
	this->pushToLog({ name, message });
}

void TextLogMenu::addToLog(std::string const& message)
{
	this->pushToLog({ std::nullopt, message });
}

void TextLogMenu::pushToLog(Message const& msg)
{
	if (this->mRecentMessages.size() == this->mDisplayableMessageCount)
	{
		this->mRecentMessages.pop_front();
	}
	this->mRecentMessages.push_back(msg);
	this->updateLogText();
}

void TextLogMenu::updateLogText()
{
	this->mpChatLog->startContent();
	auto iter = this->mRecentMessages.begin();
	while (iter != this->mRecentMessages.end())
	{
		if (iter != this->mRecentMessages.begin())
		{
			this->mpChatLog->addSegment("\n");
		}

		if (iter->senderName)
		{
			std::stringstream ss;
			ss << iter->senderName.value() << ": ";
			this->mpChatLog
				->addSegment(ss.str())
				.setSegmentColor({ 0, 1, 1, 1 });
		}

		this->mpChatLog->addSegment(iter->message);

		++iter;
	}
	this->mpChatLog->finishContent();
}
