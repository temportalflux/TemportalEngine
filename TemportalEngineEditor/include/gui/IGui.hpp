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

class IGui : public std::enable_shared_from_this<IGui>
{

public:
	IGui();
	IGui(std::string title);

	void setOwner(std::weak_ptr<graphics::ImGuiRenderer> pRenderer) { this->mpOwner = pRenderer; }

	virtual void makeGui();
	bool isOpen() const;
	virtual void open();
	void openOrFocus();
	void close() { this->mbIsOpen = false; }
	void focus();

protected:
	logging::Logger* getLog() const;

	void setTitle(std::string title) { this->mTitle = title; }
	virtual std::string getId() const;
	virtual std::string getTitle() const;
	std::string titleId() const;

	virtual i32 getFlags() const { return 0; }
	virtual bool beginView();
	virtual void renderView() {}
	void endView();
	virtual bool shouldReleaseGui() const;

	bool& openRef() { return this->mbIsOpen; }
	virtual void removeGui();

private:
	std::weak_ptr<graphics::ImGuiRenderer> mpOwner;
	std::string mTitle;
	bool mbIsOpen;
	bool bFocusOnNextRender;

};

NS_END
