#pragma once

#include "gui/IGui.hpp"

#include "gui/AssetBrowser.hpp"
#include "gui/Log.hpp"
#include "gui/modal/NewAsset.hpp"
#include "gui/modal/OpenAsset.hpp"

NS_GUI

class MainDockspace : public IGui
{

public:
	MainDockspace() = default;
	MainDockspace(std::string id, std::string title);

	void onAddedToRenderer(graphics::ImGuiRenderer *pRenderer);
	void onRemovedFromRenderer(graphics::ImGuiRenderer *pRenderer);

	void makeGui() override;

protected:
	std::string mId;

	i32 getFlags() const override;
	bool beginView() override;
	void renderView() override;

private:
	gui::AssetBrowser mAssetBrowser;
	gui::Log mLogEditor;

	gui::modal::NewAsset mModalNewProject;
	gui::modal::OpenAsset mModalOpenProject;

};

NS_END
