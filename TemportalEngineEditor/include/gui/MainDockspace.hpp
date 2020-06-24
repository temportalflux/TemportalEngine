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
	std::shared_ptr<gui::AssetBrowser> mAssetBrowser;
	std::shared_ptr<gui::Log> mLogEditor;

	std::shared_ptr<gui::modal::NewAsset> mModalNewProject;
	std::shared_ptr<gui::modal::OpenAsset> mModalOpenProject;

	std::shared_ptr<gui::modal::NewAsset> mModalNewAsset;

};

NS_END
