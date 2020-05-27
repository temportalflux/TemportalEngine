#pragma once

#include "TemportalEnginePCH.hpp"

#include "gui/IGui.hpp"
#include "types/integer.h"

NS_GUI

class MainDockspace : public IGui
{

public:
	MainDockspace();

	void makeGui() override;
	bool isOpen() const;

private:
	bool mbIsOpen;
	ui32 getId() const;
	i32 getFlags() const;

};

NS_END
