#include "gui/widget/filesystem.hpp"

#include "Editor.hpp"
#include "gui/modal/ModalFilePicker.hpp"
#include "utility/StringUtils.hpp"

#include <imgui.h>

NS_GUI

#pragma region Breadcrumbs

std::vector<std::filesystem::path> createBreadcrumbs(std::filesystem::path path, std::filesystem::path root)
{
	auto tmp = path;
	auto pathItems = std::vector<std::filesystem::path>();
	while (tmp != root)
	{
		pathItems.push_back(tmp);
		tmp = tmp.parent_path();
	}
	return std::vector<std::filesystem::path>(pathItems.rbegin(), pathItems.rend());
}

bool renderBreadcrumb(
	std::filesystem::path const &root, std::filesystem::path &path,
	std::optional<std::function<void(uIndex idx)>> OnRenderCrumb /*= std::nullopt*/
)
{
	bool bChangedPath = false;
	auto breadcrumbs = createBreadcrumbs(path, root);
	ImGui::Text("/");
	if (breadcrumbs.begin() != breadcrumbs.end())
	{
		ImGui::SameLine();
	}
	for (auto iter = breadcrumbs.begin(); iter != breadcrumbs.end(); ++iter)
	{
		ImGui::Text(iter->filename().string().c_str());
		if (ImGui::IsItemHovered())
		{
			if (ImGui::IsMouseDoubleClicked(/*mouse button*/ 0))
			{
				path = *iter;
				bChangedPath = true;
			}
			if (OnRenderCrumb)
			{
				(*OnRenderCrumb)(std::distance(breadcrumbs.begin(), iter));
			}
		}
		if (iter + 1 != breadcrumbs.end())
		{
			ImGui::SameLine();
			ImGui::Text("/");
			ImGui::SameLine();
		}
	}
	return bChangedPath;
}

#pragma endregion

#pragma region File Directory Tree

bool renderTreeSubDirectory(std::filesystem::path const &path, std::filesystem::path &lastSelectedPath);
bool renderTreeSubDirectoryContents(std::filesystem::path const &path, std::filesystem::path &lastSelectedPath);

bool renderFileDirectoryTree(char const* id, std::filesystem::path const &root, std::filesystem::path &lastSelectedPath, math::Vector2 size)
{
	if (root.empty()) return false;

	bool shouldRender = ImGui::BeginChild(id, ImVec2(size.x(), size.y()), true,
		ImGuiWindowFlags_AlwaysHorizontalScrollbar | ImGuiWindowFlags_AlwaysVerticalScrollbar
	);
	bool bChangedLastSelected = false;
	if (shouldRender)
	{
		bool bChangedPath = renderTreeSubDirectory(root, lastSelectedPath);
		if (bChangedPath) bChangedLastSelected = true;
	}
	ImGui::EndChild();

	return bChangedLastSelected;
}

bool renderTreeSubDirectory(std::filesystem::path const &path, std::filesystem::path &lastSelectedPath)
{
	bool bChangedLastSelected = false;
	bool bShowContents = ImGui::TreeNodeEx(path.stem().string().c_str(), ImGuiTreeNodeFlags_OpenOnArrow);
	if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
	{
		lastSelectedPath = path;
		bChangedLastSelected = true;
	}
	if (bShowContents)
	{
		renderTreeSubDirectoryContents(path, lastSelectedPath);
		ImGui::TreePop();
	}
	return bChangedLastSelected;
}

bool renderTreeSubDirectoryContents(std::filesystem::path const &path, std::filesystem::path &lastSelectedPath)
{
	bool bChangedLastSelected = false;
	for (const auto& entry : std::filesystem::directory_iterator(path))
	{
		if (!entry.is_directory()) continue;
		bool bChangedPath = renderTreeSubDirectory(entry.path(), lastSelectedPath);
		if (bChangedPath) bChangedLastSelected = true;
	}
	return bChangedLastSelected;
}

#pragma endregion

#pragma region Directory Viewer

bool renderDirectoryView(std::filesystem::path &path, DirectoryViewConfig const &cfg)
{
	ImGui::BeginChild(
		"directoryView", ImVec2(0, 0), false,
		ImGuiWindowFlags_AlwaysHorizontalScrollbar | ImGuiWindowFlags_AlwaysVerticalScrollbar
	);

	if (!std::filesystem::exists(path))
	{
		ImGui::Text("Missing directory, cannot render contents");
		ImGui::EndChild();
		return false;
	}
	else if (std::filesystem::is_empty(path))
	{
		ImGui::Text("Directory is empty");
		ImGui::EndChild();
		return false;
	}

	bool bChangedPath = false;

	ImGui::Columns(2); // name | ext/dir
	auto contentIter = std::filesystem::directory_iterator(path);
	for (const auto& entry : contentIter)
	{
		auto itemName = entry.path().stem().string();
		auto extension = entry.path().extension().string();
		auto bIsDirectory = entry.is_directory();
		if (!bIsDirectory)
		{
			if (cfg.CanShowFile)
			{
				if (!cfg.CanShowFile(entry.path())) continue;
			}
			else
			{
				if (!cfg.bShowHiddenFiles && itemName[0] == '.') continue;
				if (!cfg.AllowedExtensions.empty() && cfg.AllowedExtensions.find(extension) == cfg.AllowedExtensions.end()) continue;
				if (cfg.BlockedExtensions.find(extension) != cfg.BlockedExtensions.end()) continue;
			}
		}

		ImGui::Text(itemName.c_str());
		if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(/* mouse button */ 0))
		{
			if (bIsDirectory)
			{
				path = entry.path();
				bChangedPath = true;
			}
			else if (cfg.OnFileOpen && entry.is_regular_file())
			{
				cfg.OnFileOpen(entry.path());
			}
		}

		// Context Menu
		if (entry.is_regular_file())
		{
			if (!cfg.FileContextMenuItems.empty() && ImGui::BeginPopupContextItem(entry.path().string().c_str()))
			{
				for (auto const& item : cfg.FileContextMenuItems)
				{
					if (item.onSelected && ImGui::Selectable(item.optionLabelId.c_str()))
					{
						item.onSelected(entry.path());
					}
				}
				ImGui::EndPopup();
			}
		}
		else if (bIsDirectory)
		{
			if (!cfg.DirectoryContextMenuItems.empty() && ImGui::BeginPopupContextItem(entry.path().string().c_str()))
			{
				for (auto const& item : cfg.DirectoryContextMenuItems)
				{
					if (item.onSelected && ImGui::Selectable(item.optionLabelId.c_str()))
					{
						item.onSelected(entry.path());
					}
				}
				ImGui::EndPopup();
			}
		}

		ImGui::NextColumn();
		if (bIsDirectory)
		{
			ImGui::Text("directory");
		}
		else if (entry.is_regular_file())
		{
			ImGui::Text(extension.c_str());
		}
		else
		{
			ImGui::Text("unsupported type");
		}
		ImGui::NextColumn();

	}

	ImGui::EndChild();

	return bChangedPath;
}

#pragma endregion

std::string PathText::get() const
{
	return utility::createStringFromFixedArray(this->rawContent);
}

std::filesystem::path PathText::path() const
{
	return this->root / this->get();
}

void PathText::setPath(std::filesystem::path const &path)
{
	memcpy_s(this->rawContent.data(), this->rawContent.size() * sizeof(char), path.string().data(), path.string().length() * sizeof(char));
}

bool renderFileSelectorField(std::string const titleId, FileSelectorField &cfg)
{
	bool bChanged = ImGui::InputText(titleId.c_str(), cfg.rawContent.data(), cfg.rawContent.size(), cfg.flags);
	ImGui::SameLine();
	if (ImGui::Button("..."))
	{
		std::shared_ptr<gui::modal::FilePicker> modal = Editor::EDITOR->openNewGui<gui::modal::FilePicker>("File Picker");
		modal->setRoot(cfg.root);
		modal->setConfig(cfg.directoryViewCfg);
	}
	return bChanged;
}

NS_END
