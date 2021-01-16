#include "ui/InputWidget.hpp"

#include "Engine.hpp"
#include "graphics/Command.hpp"
#include "input/InputCore.hpp"
#include "logging/Logger.hpp"

using namespace ui;

logging::Logger UI_LOG = DeclareLog("UI", LOG_INFO);

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
	this->startContent();
	this->mCursorPos = 0;
	this->mContentSegment = { "", math::Color(1) };
	this->mCursorSegment = { "", { 1, 0, 0, 1 } };
	this->uncommittedSegments() = { this->mContentSegment };
	this->uncommittedContentLength() = (ui32)this->mContentSegment.content.length();
	this->finishContent();
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
				if (this->mContentSegment.content.length() > 0 && this->mCursorPos > 0)
				{
					this->lock();
					this->mContentSegment.content.erase(this->mCursorPos - 1, 1);
					this->mCursorPos--;
					this->uncommittedSegments() = { this->mContentSegment };
					this->uncommittedContentLength() = (ui32)this->mContentSegment.content.length();
					this->markDirty();
					this->unlock();
				}
			}
			if (evt.inputKey.key == input::EKey::SP_DELETE)
			{
				if (this->mCursorPos < this->mContentSegment.content.length())
				{
					this->lock();
					this->mContentSegment.content.erase(this->mCursorPos, 1);
					this->uncommittedSegments() = { this->mContentSegment };
					this->uncommittedContentLength() = (ui32)this->mContentSegment.content.length();
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
				if (this->mCursorPos < this->mContentSegment.content.length())
				{
					this->lock();
					this->mCursorPos++;
					this->markDirty();
					this->unlock();
				}
			}
			if (evt.inputKey.key == input::EKey::RETURN && this->mContentSegment.content.length() > 0)
			{
				this->onConfirm.execute(this->mContentSegment.content);
			}
		}
	}
	if (
		evt.type == input::EInputType::TEXT
		&& this->mContentSegment.content.length() < this->maxContentLength())
	{
		this->lock();
		this->mContentSegment.content.insert(
			this->mContentSegment.content.begin() + this->mCursorPos,
			evt.inputText.text[0]
		);
		this->mCursorPos++;
		this->uncommittedSegments() = { this->mContentSegment };
		this->uncommittedContentLength() = (ui32)this->mContentSegment.content.length();
		this->markDirty();
		this->unlock();
	}
}

ui32 Input::desiredCharacterCount() const
{
	return Text::desiredCharacterCount() + 1; // +1 for the cursor
}

Text::Segment const& Input::segmentAt(uIndex idxSegment, uIndex idxSegmentChar, uIndex idxTotalChar) const
{
	if (idxTotalChar == this->mCursorPos) return this->mCursorSegment;
	else return Text::segmentAt(idxSegment, idxSegmentChar, idxTotalChar);
}

char Input::charAt(uIndex idxSegment, uIndex idxSegmentChar, uIndex idxTotalChar) const
{
	if (idxTotalChar == this->mCursorPos) return '|';
	return Text::charAt(
		idxSegment, idxSegmentChar,
		(
			idxTotalChar < this->mCursorPos
			? idxTotalChar
			: idxTotalChar - 1
		)
	);
}

bool Input::incrementChar(uIndex &idxSegment, uIndex &idxSegmentChar, uIndex idxTotalChar) const
{
	// if the next position is our custom character, ensure the iteration does not finish
	if (idxTotalChar + 1 == this->mCursorPos) return false;
	// otherwise, the cursor is not up next, so default to Text
	return Text::incrementChar(idxSegment, idxSegmentChar, idxTotalChar);
}
