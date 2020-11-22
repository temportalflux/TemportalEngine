#pragma once

#include "TemportalEnginePCH.hpp"
#include "gui/IGui.hpp"

NS_GUI

class AssetReferenceViewer : public IGui
{

public:
	AssetReferenceViewer() = default;
	AssetReferenceViewer(std::string title);
	~AssetReferenceViewer();

	void setAssetFilePath(std::filesystem::path const& absolutePath);

	void renderView() override;

protected:
	i32 getFlags() const override;

private:
	std::filesystem::path mAbsolutePath;

};

NS_END
