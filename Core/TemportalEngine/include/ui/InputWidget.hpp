#pragma once

#include "ui/TextWidget.hpp"

#include "input/InputListener.hpp"
#include "Delegate.hpp"

NS_UI

class Input
	: public ui::Text
	, public input::Listener
{

public:
	Input();

	ExecuteDelegate<void(std::string)> onConfirm;

	void setActive(bool bActive);
	void clear();

protected:
	ui32 desiredCharacterCount() const override;
	Segment const& segmentAt(uIndex idxSegment, uIndex idxSegmentChar, uIndex idxTotalChar) const override;
	char charAt(uIndex idxSegment, uIndex idxSegmentChar, uIndex idxTotalChar) const override;
	bool incrementChar(uIndex &idxSegment, uIndex &idxSegmentChar, uIndex idxTotalChar) const override;

private:
	bool mbIsActive;
	uIndex mCursorPos;
	Text::Segment mContentSegment;
	Text::Segment mCursorSegment;

	void onInput(input::Event const& evt) override;

};

NS_END
