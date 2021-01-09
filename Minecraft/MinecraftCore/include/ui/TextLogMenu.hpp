#pragma once

#include "CoreInclude.hpp"
#include "input/Event.hpp"
#include "ui/Core.hpp"
#include "ui/ImageWidget.hpp"
#include "ui/TextWidget.hpp"

FORWARD_DEF(NS_INPUT, class Queue);

NS_UI
class WidgetRenderer;

class TextLogMenu : public std::enable_shared_from_this<TextLogMenu>
{
	
public:
	TextLogMenu();
	~TextLogMenu();

	void addImagesToRenderer(ui::WidgetRenderer *renderer);
	bool isVisible() const;
	void setIsVisible(bool bVisible);

	void bindInput(std::shared_ptr<input::Queue> pInput);
	void unbindInput(std::shared_ptr<input::Queue> pInput);

private:
	bool mbIsVisible;
	std::shared_ptr<ui::Image> mpInputBarBkgd;
	std::shared_ptr<ui::Image> mpLogBkgd;
	std::shared_ptr<ui::Text> mpInputText;

	std::string mInputContent;
	ui32 mInputCursorPos;

	std::shared_ptr<ui::Image> mpBackgroundDemo;
	std::vector<std::shared_ptr<ui::Image>> mSlots;

	void onInputKey(input::Event const& evt);
	void onInputText(input::Event const& evt);

	void removeInputAtCursor();
	void removeInputAfterCursor();

};

NS_END
