#pragma once

#include "gui/IGui.hpp"

#define NS_MODAL namespace modal {

NS_GUI NS_MODAL

class Modal : public IGui
{

public:
	Modal() = default;
	Modal(std::string title);

	void open() override;

protected:
	i32 getFlags() const override { return 0; }
	void makeGui() override;
	virtual void drawContents() = 0;

private:
	bool bOpenOnNextFrame;

};

NS_END NS_END
