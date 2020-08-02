#pragma once

#include "TemportalEnginePCH.hpp"

NS_GUI

/**
 * Renders a set of breadcrumbs for a filesystem path.
 */
bool renderBreadcrumb(std::filesystem::path const &root, std::filesystem::path &path, std::optional<std::function<void(uIndex idx)>> OnRenderCrumb = std::nullopt);

/**
 * Renders a tree of directories descending from `root`.
 * Returns true and sets `lastSelectedPath` if the user double clicks on any entry.
 */
bool renderFileDirectoryTree(char const* id, std::filesystem::path const &root, std::filesystem::path &lastSelectedPath, math::Vector2 size);

struct DirectoryViewConfig
{
	bool bShowHiddenFiles = false;
	std::unordered_set<std::string> AllowedExtensions;
	std::unordered_set<std::string> BlockedExtensions;
	std::function<bool(std::filesystem::path const &path)> CanShowFile;

	bool bDirectoryOnlySelection = false;
	std::function<void(std::filesystem::path const &path)> OnFileOpen;

	struct ContextMenuItem
	{
		std::string optionLabelId;
		std::function<void(std::filesystem::path const &path)> onSelected;
	};
	std::vector<ContextMenuItem> FileContextMenuItems;
	std::vector<ContextMenuItem> DirectoryContextMenuItems;
};

/**
 * Renders a viewer of a directory.
 */
bool renderDirectoryView(std::filesystem::path &path, DirectoryViewConfig const &cfg);

struct PathText
{
	std::filesystem::path root;
	std::array<char, 256> rawContent;

	std::string get() const;
	std::filesystem::path path() const;
	void setPath(std::filesystem::path const &path);
};

struct FileSelectorField : public PathText
{
	ui32 flags;
	DirectoryViewConfig directoryViewCfg;
};

bool renderFileSelectorField(std::string const titleId, FileSelectorField &cfg);

NS_END
