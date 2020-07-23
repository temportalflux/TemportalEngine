#pragma once

#include "gui/IGui.hpp"

#include "gui/modal/NewAsset.hpp"

#include <filesystem>

NS_GUI

class AssetBrowser : public IGui
{

public:
	AssetBrowser() = default;
	AssetBrowser(std::string title);

	void open() override;
	void renderView() override;

protected:
	i32 getFlags() const override;

private:
	bool bShowingNonAssets;
	std::filesystem::path mDefaultPath;
	std::filesystem::path mCurrentPath;
	std::vector<std::filesystem::path> mBreadcrumbs;

	void setPath(std::filesystem::path path);
	std::filesystem::path getCurrentRelativePath() const;

	void renderMenuBar();
	void renderBreadcrumbs();
	void renderDirectoryContents();

};

NS_END
