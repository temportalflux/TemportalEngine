#include "gui/AssetBrowser.hpp"

#include "logging/Logger.hpp"

#include "Editor.hpp"
#include "asset/AssetManager.hpp"

#include <algorithm>
#include <functional>
#include <optional>
#include <string>
#include <vector>
#include <imgui.h>

using namespace gui;

AssetBrowser::AssetBrowser(std::string title)
	: IGui(title)
{
	this->bShowingNonAssets = false;
}

void AssetBrowser::open()
{
	if (Editor::EDITOR->hasProject())
	{
		this->mDefaultPath = Editor::EDITOR->getCurrentAssetDirectory();
		this->setPath(this->mDefaultPath);
	}
	IGui::open();
}

i32 AssetBrowser::getFlags() const
{
	return ImGuiWindowFlags_MenuBar;
}

void AssetBrowser::renderView()
{
	if (!Editor::EDITOR->hasProject()) return;

	this->renderMenuBar();

	if (this->mCurrentPath != this->mDefaultPath)
	{
		if (ImGui::ArrowButton("Back", ImGuiDir_Left))
		{
			this->setPath(this->mCurrentPath.parent_path());
		}
		ImGui::SameLine();
	}
	this->renderBreadcrumbs();	
	
	ImGui::Separator();
	
	this->renderDirectoryContents();
}

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

void AssetBrowser::setPath(std::filesystem::path path)
{
	this->mCurrentPath = path;
	this->mBreadcrumbs = createBreadcrumbs(this->mCurrentPath, Editor::EDITOR->getProject()->getAbsoluteDirectoryPath());
}

std::filesystem::path AssetBrowser::getCurrentRelativePath() const
{
	return std::filesystem::relative(this->mCurrentPath, Editor::EDITOR->getProject()->getAbsoluteDirectoryPath());
}

void AssetBrowser::renderMenuBar()
{
	if (ImGui::BeginMenuBar())
	{
		if (ImGui::MenuItem("New Asset", "", false, true))
		{
			auto gui = Editor::EDITOR->openNewGui<gui::modal::NewAsset>("New Asset");
			gui->setRoot(Editor::EDITOR->getProject()->getAssetDirectory());
			gui->setDefaultPath(this->getCurrentRelativePath());
			gui->setCallback([](auto asset) { Editor::EDITOR->openAssetEditor(asset); });
		}
		if (ImGui::MenuItem("New Folder", "", false, true))
			std::filesystem::create_directory(this->mCurrentPath / "New Folder");
		if (ImGui::MenuItem(((this->bShowingNonAssets ? "Hide" : "Show") + std::string(" Misc Files###AssetToggle")).c_str(), "", false, true))
			this->bShowingNonAssets = !this->bShowingNonAssets;
		ImGui::EndMenuBar();
	}
}

void AssetBrowser::renderBreadcrumbs()
{
	std::optional<std::filesystem::path> newPath = std::nullopt;
	for (auto iter = this->mBreadcrumbs.begin(); iter != this->mBreadcrumbs.end(); ++iter)
	{
		ImGui::Text(iter->filename().string().c_str());
		if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(/*mouse button*/ 0))
		{
			newPath = *iter;
		}
		if (iter == this->mBreadcrumbs.begin())
		{
			if (ImGui::IsItemHovered())
			{
				auto project = Editor::EDITOR->getProject();
				ImGui::SetTooltip(("Absolute Root: " + project->getAbsoluteDirectoryPath().string()).c_str());
			}
		}
		if (iter + 1 != this->mBreadcrumbs.end())
		{
			ImGui::SameLine();
			ImGui::Text("/");
			ImGui::SameLine();
		}
	}
	if (newPath.has_value())
	{
		this->setPath(newPath.value());
	}
}

void AssetBrowser::renderDirectoryContents()
{
	if (!std::filesystem::exists(this->mCurrentPath))
	{
		ImGui::Text("Missing directory, cannot render contents");
		return;
	}
	else if (std::filesystem::is_empty(this->mCurrentPath))
	{
		ImGui::Text("Directory is empty");
		return;
	}

	// TODO: Provide renaming of folders and assets
	// TODO: Add context menu option for compiling an asset w/o opening the editor

	auto assetManager = asset::AssetManager::get();
	std::optional<std::filesystem::path> newPath = std::nullopt;
	ImGui::Columns(2); // name | ext/dir
	for (const auto& entry : std::filesystem::directory_iterator(this->mCurrentPath))
	{
		auto itemName = entry.path().stem().string();
		if (itemName[0] == '.') continue;
		auto bIsDirectory = entry.is_directory();
		if (!bIsDirectory && !this->bShowingNonAssets && !assetManager->isValidAssetExtension(entry.path().extension().string())) continue;
		bool bIsAssetAndOpenForEdit = false;
		auto entryIsEmpty = std::filesystem::is_empty(entry.path());

		ImGui::Text(itemName.c_str());
		if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(/* mouse button */ 0))
		{
			if (bIsDirectory) newPath = entry.path();
			else if (entry.is_regular_file()) bIsAssetAndOpenForEdit = true;
		}
		if ((entry.is_regular_file() || entryIsEmpty)
				&& ImGui::BeginPopupContextItem(entry.path().string().c_str()))
		{
			if (!bIsDirectory && ImGui::Selectable("Edit")) bIsAssetAndOpenForEdit = true;
			if (ImGui::Selectable("Delete")) asset::AssetManager::get()->deleteFile(entry.path());
			ImGui::EndPopup();
		}

		ImGui::NextColumn();
		if (bIsDirectory)
		{
			ImGui::Text("directory");
		}
		else if (entry.is_regular_file())
		{
			ImGui::Text(entry.path().extension().string().c_str());
		}
		else
		{
			ImGui::Text("unsupported type");
		}
		ImGui::NextColumn();

		if (bIsAssetAndOpenForEdit)
		{
			Editor::EDITOR->openAssetEditor(asset::readAssetFromDisk(entry.path(), asset::EAssetSerialization::Json));
		}
	}
	if (newPath.has_value())
	{
		this->setPath(newPath.value());
	}
}
