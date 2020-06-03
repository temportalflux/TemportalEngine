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
	void makeGui() override;
	void renderView() override;

protected:
	i32 getFlags() const override;

private:
	std::filesystem::path mDefaultPath;
	std::filesystem::path mCurrentPath;
	std::vector<std::filesystem::path> mBreadcrumbs;

	gui::modal::NewAsset mModalNewAsset;

	void setPath(std::filesystem::path path);
	std::filesystem::path getCurrentRelativePath() const;

	void renderMenuBar();
	void renderBreadcrumbs();
	void renderDirectoryContents();

};

NS_END
