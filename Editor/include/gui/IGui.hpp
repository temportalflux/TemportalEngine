#pragma once

#include "TemportalEnginePCH.hpp"

#include "types/integer.h"

#include <string>

NS_GRAPHICS
class ImGuiRenderer;
NS_END

NS_LOGGING
class Logger;
NS_END

NS_GUI

class IGui
{

public:
	IGui();
	IGui(std::string title);

	virtual void onAddedToRenderer(graphics::ImGuiRenderer *pRenderer) {}
	virtual void onRemovedFromRenderer(graphics::ImGuiRenderer *pRenderer) {}

	virtual void makeGui();
	bool isOpen() const;
	void open();
	void openOrFocus();

protected:
	logging::Logger* getLog() const;

	virtual i32 getFlags() const = 0;
	virtual bool beginView();
	virtual void renderView() = 0;
	void endView();

private:
	std::string mTitle;
	bool mbIsOpen;

};

NS_END
