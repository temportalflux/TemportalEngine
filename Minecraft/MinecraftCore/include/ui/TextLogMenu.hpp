#pragma once

#include "CoreInclude.hpp"
#include "input/InputListener.hpp"
#include "ui/Core.hpp"
#include "ui/ImageWidget.hpp"
#include "ui/TextWidget.hpp"
#include "ui/InputWidget.hpp"

#include <queue>

NS_UI
class WidgetRenderer;

class TextLogMenu : public input::Listener
{
	
public:
	TextLogMenu();
	~TextLogMenu();

	void init(ui::WidgetRenderer *renderer);
	bool isVisible() const;
	void setIsVisible(bool bVisible);

	void onMessageReceived(std::optional<ui32> senderNetId, std::string const& message);
	void addToLog(std::string const& message);

private:
	bool mbIsVisible;
	std::shared_ptr<ui::Image> mpInputBarBkgd;
	std::shared_ptr<ui::Image> mpLogBkgd;
	std::shared_ptr<ui::Input> mpInputText;

	struct Message
	{
		std::optional<ui32> senderNetId;
		std::string message;
	};
	ui8 mDisplayableMessageCount;
	std::deque<Message> mRecentMessages;
	std::shared_ptr<ui::Text> mpChatLog;

	std::shared_ptr<ui::Image> mpBackgroundDemo;
	std::vector<std::shared_ptr<ui::Image>> mSlots;

	void onInput(input::Event const& evt) override;
	void onInputConfirmed(std::string input);

	void pushToLog(Message const& msg);
	void updateLogText();

};

NS_END
