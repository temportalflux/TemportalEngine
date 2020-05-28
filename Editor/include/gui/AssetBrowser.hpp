#pragma once

#include "gui/IGui.hpp"

NS_GUI

class AssetBrowser : public IGui
{

public:
	AssetBrowser() = default;
	AssetBrowser(std::string title);

	void renderView() override;

protected:
	i32 getFlags() const override;

};

NS_END
