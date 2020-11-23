#include "gui/AssetBrowser.hpp"

#include "logging/Logger.hpp"

#include "Editor.hpp"
#include "Engine.hpp"
#include "asset/AssetManager.hpp"
#include "gui/AssetReferenceViewer.hpp"

#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>

using namespace gui;

AssetBrowser::AssetBrowser(std::string title)
	: IGui(title)
{
	this->bShowingNonAssets = false;
	this->mViewConfig.bShowHiddenFiles = false;
	
	this->mViewConfig.OnFileOpen = std::bind(&AssetBrowser::onFileOpen, this, std::placeholders::_1);
	this->mViewConfig.CanShowFile = std::bind(&AssetBrowser::canShowFileInView, this, std::placeholders::_1);
	
	DirectoryViewConfig::ContextMenuItem onPathDelete = { "Delete", std::bind(&AssetBrowser::onPathDelete, this, std::placeholders::_1) };
	this->mViewConfig.FileContextMenuItems.push_back({ "Edit", this->mViewConfig.OnFileOpen });
	this->mViewConfig.FileContextMenuItems.push_back({ "View References", std::bind(&AssetBrowser::onViewReferences, this, std::placeholders::_1) });
	this->mViewConfig.FileContextMenuItems.push_back(onPathDelete);
	this->mViewConfig.DirectoryContextMenuItems.push_back(onPathDelete);

	this->mViewConfig.startDragDrop = std::bind(&AssetBrowser::onStartDragDrop, this, std::placeholders::_1);
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

	gui::renderBreadcrumb(
		Editor::EDITOR->getProject()->getAbsoluteDirectoryPath(), this->mCurrentPath,
		[](uIndex idx)
		{
			if (idx == 0)
			{
				ImGui::SetTooltip(("Absolute Root: " + Editor::EDITOR->getProject()->getAbsoluteDirectoryPath().string()).c_str());
			}
		}
	);
	
	ImGui::Separator();

	renderDirectoryTree();
}

void AssetBrowser::renderDirectoryTree()
{
	auto& path = this->mCurrentPath;
	
	ImGui::BeginChild(
		"directoryView", ImVec2(0, 0), false,
		ImGuiWindowFlags_AlwaysHorizontalScrollbar | ImGuiWindowFlags_AlwaysVerticalScrollbar
	);

	if (!std::filesystem::exists(path))
	{
		ImGui::Text("Missing directory, cannot render contents");
		ImGui::EndChild();
		return;
	}
	else if (std::filesystem::is_empty(path))
	{
		ImGui::Text("Directory is empty");
		ImGui::EndChild();
		return;
	}
	for (ui8 i = 0; i < ImGuiMouseButton_COUNT; ++i)
	{
		if (ImGui::IsMouseReleased(i)) getLog()->log(LOG_INFO, "Release %i", i);
	}
	renderDirectoryContents(path);
	ImGui::EndChild();
}

void AssetBrowser::renderDirectoryItem(std::filesystem::path const& path)
{
	auto const itemName = path.stem().string();
	if (itemName[0] == '.') return;

	bool bIsDirectory = std::filesystem::is_directory(path);
	bool bIsHovered = false;
	if (!bIsDirectory && !this->canShowFileInView(path)) return;
	if (this->mRenamingAsset == path)
	{
		if (ImGui::InputText(
			("###" + itemName).c_str(), &this->mRenamingStr,
			ImGuiInputTextFlags_EnterReturnsTrue
			| ImGuiInputTextFlags_CharsNoBlank
			| ImGuiInputTextFlags_AutoSelectAll
		))
		{
			if (!bIsDirectory) asset::AssetManager::get()->renameAsset(path, this->mRenamingStr);
			else if (std::filesystem::is_empty(path)) std::filesystem::rename(path, path.parent_path() / this->mRenamingStr);
			this->mRenamingAsset = std::filesystem::path();
			this->mRenamingStr = std::string();
		}
		ImGui::SetItemDefaultFocus();
	}
	else if (!bIsDirectory)
	{
		ImGui::Text(itemName.c_str());
		bIsHovered = ImGui::IsItemHovered();
		this->onStartDragDrop(path);
	}
	else
	{
		bool bShowContents = ImGui::TreeNode(itemName.c_str());
		bIsHovered = ImGui::IsItemHovered();
		this->onStartDragDrop(path);
		if (bShowContents)
		{
			renderDirectoryContents(path);
			ImGui::TreePop();
		}
	}
	if (bIsHovered && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) this->onFileOpen(path);
	if (bIsHovered && ImGui::IsMouseReleased(ImGuiMouseButton_Right))
	{
		ImGui::OpenPopup(itemName.c_str());
	}
	if (ImGui::BeginPopup(itemName.c_str()))
	{
		if (!bIsDirectory && ImGui::Selectable("Edit")) this->onFileOpen(path);
		if (!bIsDirectory && ImGui::Selectable("View References")) this->onViewReferences(path);
		if ((!bIsDirectory || std::filesystem::is_empty(path)) && ImGui::Selectable("Rename"))
		{
			this->mRenamingAsset = path;
			this->mRenamingStr = path.stem().string();
		}
		if (ImGui::Selectable("Delete")) this->onPathDelete(path);
		ImGui::EndPopup();
	}
}

void AssetBrowser::renderDirectoryContents(std::filesystem::path const& path)
{
	for (auto const& entry : std::filesystem::directory_iterator(path))
	{
		renderDirectoryItem(entry.path());
	}
}

void AssetBrowser::setPath(std::filesystem::path path)
{
	this->mCurrentPath = path;
}

std::filesystem::path AssetBrowser::getCurrentRelativePath() const
{
	auto assetDirPath = Editor::EDITOR->getProject()->getAssetDirectory();
	return this->mCurrentPath == assetDirPath ? "" : std::filesystem::relative(this->mCurrentPath, assetDirPath);
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

bool isAnAsset(std::filesystem::path const &path)
{
	auto extension = path.extension().string();
	auto assetManager = engine::Engine::Get()->getAssetManager();
	return assetManager->isValidAssetExtension(extension);
}

bool AssetBrowser::canShowFileInView(std::filesystem::path const &path)
{
	return isAnAsset(path) || this->bShowingNonAssets;
}

void AssetBrowser::onFileOpen(std::filesystem::path const &path)
{
	if (isAnAsset(path))
	{
		Editor::EDITOR->openAssetEditor(asset::readAssetFromDisk(path, asset::EAssetSerialization::Json));
	}
}

void AssetBrowser::onPathDelete(std::filesystem::path const &path)
{
	if (std::filesystem::is_directory(path))
	{
		auto amountDeleted = std::filesystem::remove_all(path);
		getLog()->log(LOG_INFO, "Deleted %i items at %s", amountDeleted, path.filename().string().c_str());
		return;
	}

	if (isAnAsset(path))
	{
		auto manager = asset::AssetManager::get();
		if (manager->isAssetReferenced(path))
		{
			getLog()->log(LOG_INFO, "Cannot delete %s, it is referenced by other assets.", path.filename().string().c_str());
			this->onViewReferences(path);
		}
		else
		{
			manager->deleteFile(path);
			getLog()->log(LOG_INFO, "Deleted %s", path.filename().string().c_str());
		}
		return;
	}

	std::filesystem::remove(path);
	getLog()->log(LOG_INFO, "Deleted %s", path.filename().string().c_str());
}

void AssetBrowser::onStartDragDrop(std::filesystem::path const &path)
{
	auto assetManager = engine::Engine::Get()->getAssetManager();
	if (std::filesystem::is_directory(path))
	{
		if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
		{
			auto pathStr = path.string();
			ImGui::SetDragDropPayload("DIRECTORY", &pathStr, sizeof(std::string));
			ImGui::Text(path.stem().string().c_str());
			ImGui::EndDragDropSource();
		}
		if (ImGui::BeginDragDropTarget())
		{
			if (ImGuiPayload const *payload = ImGui::GetDragDropPayload())
			{
				if (payload->IsDataType("ASSETPATH"))
				{
					if (payload = ImGui::AcceptDragDropPayload("ASSETPATH", ImGuiDragDropFlags_None))
					{
						assetManager->moveAsset(*((asset::AssetPath*)(payload->Data)), path);
					}
				}
			}
			ImGui::EndDragDropTarget();
		}
	}
	if (isAnAsset(path))
	{
		asset::AssetPath* assetPathPtr = assetManager->getAssetMetadataPtr(path);
		if (assetPathPtr != nullptr && ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
		{
			ImGui::SetDragDropPayload("ASSETPATH", assetPathPtr, sizeof(*assetPathPtr));
			ImGui::Text(assetPathPtr->toShortName().c_str());
			ImGui::EndDragDropSource();
		}
	}
}

void AssetBrowser::onViewReferences(std::filesystem::path const &path)
{
	auto viewer = Editor::EDITOR->openNewGui<gui::AssetReferenceViewer>("Reference Viewer: " + path.filename().stem().string());
	viewer->setAssetFilePath(path);
}
