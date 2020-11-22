#pragma once

#include "gui/IGui.hpp"

#include "gui/modal/NewAsset.hpp"
#include "gui/widget/filesystem.hpp"

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
	DirectoryViewConfig mViewConfig;

	void setPath(std::filesystem::path path);
	std::filesystem::path getCurrentRelativePath() const;

	void renderMenuBar();
	bool canShowFileInView(std::filesystem::path const &path);
	void onFileOpen(std::filesystem::path const &path);
	void onPathDelete(std::filesystem::path const &path);
	void onStartDragDrop(std::filesystem::path const &path);
	void onViewReferences(std::filesystem::path const &path);

};

NS_END
