#include "ui/InputWidget.hpp"

#include "Engine.hpp"
#include "graphics/Command.hpp"
#include "input/InputCore.hpp"
#include "logging/Logger.hpp"

using namespace ui;

logging::Logger UI_LOG = DeclareLog("UI");

Input::Input()
	: ui::Text()
	, input::Listener()
	, mbIsActive(false)
{
}

void Input::setActive(bool bActive)
{
	if (this->mbIsActive == bActive) return;
	this->mbIsActive = bActive;

	this->clear();

	// TODO: multiple text fields will require this to be a counter instead of just a global state
	if (bActive) input::startTextInput();
	else input::stopTextInput();

	if (bActive)
	{
		this->startListening(input::EInputType::KEY);
		this->startListening(input::EInputType::TEXT);
	}
	else
	{
		this->stopListening(input::EInputType::KEY);
		this->stopListening(input::EInputType::TEXT);
	}
}

void Input::clear()
{
	this->mCursorPos = 0;
	this->mFieldContent = "";
	this->setContent(this->mFieldContent);
}

void Input::onInput(input::Event const& evt)
{
	if (evt.type == input::EInputType::KEY)
	{
		auto bIsPressed = evt.inputKey.action == input::EAction::PRESS;
		auto bIsRepeat = evt.inputKey.action == input::EAction::REPEAT;
		if (bIsPressed || bIsRepeat)
		{
			if (evt.inputKey.key == input::EKey::BACKSPACE)
			{
				if (this->mFieldContent.length() > 0 && this->mCursorPos > 0)
				{
					this->lock();
					this->mFieldContent.erase(this->mCursorPos - 1, 1);
					this->mCursorPos--;
					this->uncommittedContent() = this->mFieldContent;
					this->markDirty();
					this->unlock();
				}
			}
			if (evt.inputKey.key == input::EKey::SP_DELETE)
			{
				if (this->mCursorPos < this->mFieldContent.length())
				{
					this->lock();
					this->mFieldContent.erase(this->mCursorPos, 1);
					this->uncommittedContent() = this->mFieldContent;
					this->markDirty();
					this->unlock();
				}
			}
			if (evt.inputKey.key == input::EKey::LEFT)
			{
				if (this->mCursorPos > 0)
				{
					this->lock();
					this->mCursorPos--;
					this->markDirty();
					this->unlock();
				}
			}
			if (evt.inputKey.key == input::EKey::RIGHT)
			{
				if (this->mCursorPos < this->mFieldContent.length())
				{
					this->lock();
					this->mCursorPos++;
					this->markDirty();
					this->unlock();
				}
			}
			if (evt.inputKey.key == input::EKey::RETURN && this->mFieldContent.length() > 0)
			{
				this->onConfirm.execute(this->mFieldContent);
			}
		}
	}
	if (evt.type == input::EInputType::TEXT && this->mFieldContent.length() < this->maxContentLength())
	{
		this->lock();
		this->mFieldContent.insert(this->mFieldContent.begin() + this->mCursorPos, evt.inputText.text[0]);
		this->mCursorPos++;
		this->uncommittedContent() = this->mFieldContent;
		this->markDirty();
		this->unlock();
	}
}

ui32 Input::desiredCharacterCount() const
{
	return Text::desiredCharacterCount() + 1; // +1 for the cursor
}

uSize Input::contentLength() const
{
	return Text::contentLength() + 1;
}

char Input::charAt(uIndex i) const
{
	if (i == this->mCursorPos) return '|';
	else if (i < this->mCursorPos) return Text::charAt(i);
	else return Text::charAt(i - 1);
}
