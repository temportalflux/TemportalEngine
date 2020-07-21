#pragma once

#include "TemportalEnginePCH.hpp"

NS_GUI

/**
 * A tree of directories contained within a set root directory.
 */
class FileDirectoryTree
{

public:
	void setRootDirectory(std::filesystem::path const &path);
	void render();
	std::optional<std::filesystem::path> const& getLastSelectedPath() const;

private:
	std::filesystem::path mRootDirPath;
	std::optional<std::filesystem::path> mPathSelectedDuringRender;

	// selectedPath: the path of the directory node that was clicked but whose toggled/opened state was not changed
	void renderSubDirectory(std::filesystem::path const &path);
	void renderSubDirectoryContents(std::filesystem::path const &path);

};

NS_END
