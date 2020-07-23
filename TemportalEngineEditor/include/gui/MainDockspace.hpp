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

	void makeGui() override;

protected:
	std::string mId;

	i32 getFlags() const override;
	bool beginView() override;
	void renderView() override;

private:
	bool mbIsBuildingAssets;

	std::weak_ptr<gui::AssetBrowser> mpAssetBrowser;
	std::weak_ptr<gui::Log> mpEditorLog;

	std::function<void(std::shared_ptr<asset::Asset> asset)> mOnAssetCreated;
	std::function<void(std::shared_ptr<asset::Asset> asset)> mOnProjectOpenedOrCreated;

};

NS_END
