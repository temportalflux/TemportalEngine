#pragma once

#include "gui/IGui.hpp"

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
	std::filesystem::path mCurrentPath;
	std::vector<std::filesystem::path> mBreadcrumbs;

	void setPath(std::filesystem::path path);
	void renderBreadcrumbs();

};

NS_END
