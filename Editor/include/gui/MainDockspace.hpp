#pragma once

#include "gui/IGui.hpp"

#include "gui/AssetBrowser.hpp"

NS_GUI

class MainDockspace : public IGui
{

public:
	MainDockspace() = default;
	MainDockspace(std::string id, std::string title);

	void onAddedToRenderer(graphics::ImGuiRenderer *pRenderer);
	void onRemovedFromRenderer(graphics::ImGuiRenderer *pRenderer);

protected:
	std::string mId;
	i32 getFlags() const override;
	bool beginView() override;
	void renderView() override;

private:
	gui::AssetBrowser mAssetBrowser;

};

NS_END
