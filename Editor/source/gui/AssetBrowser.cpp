#include "gui/AssetBrowser.hpp"

#include "logging/Logger.hpp"

#include "Editor.hpp"

#include <algorithm>
#include <optional>
#include <string>
#include <vector>
#include <imgui.h>

using namespace gui;

AssetBrowser::AssetBrowser(std::string title) : IGui(title)
{
}

void AssetBrowser::open()
{
	if (Editor::EDITOR->hasProject())
	{
		auto project = Editor::EDITOR->getProject();
		this->mDefaultPath = project->getAssetDirectory();
		this->setPath(this->mDefaultPath);
	}
	IGui::open();
}

i32 AssetBrowser::getFlags() const
{
	return ImGuiWindowFlags_None;
}

void AssetBrowser::renderView()
{
	if (!Editor::EDITOR->hasProject()) return;

	if (this->mCurrentPath != this->mDefaultPath)
	{
		if (ImGui::ArrowButton("", ImGuiDir_Left))
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

	auto project = Editor::EDITOR->getProject();
	auto root = project->getAbsoluteDirectoryPath();
	this->mBreadcrumbs = createBreadcrumbs(this->mCurrentPath, project->getAbsoluteDirectoryPath());
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

	std::optional<std::filesystem::path> newPath = std::nullopt;
	ImGui::Columns(2); // name | ext/dir
	for (const auto& entry : std::filesystem::directory_iterator(this->mCurrentPath))
	{
		auto itemName = entry.path().stem().string();
		auto bIsDirectory = entry.is_directory();

		ImGui::Text(itemName.c_str());
		if (bIsDirectory && ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(/* mouse button */ 0))
		{
			newPath = entry.path();
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
	}
	if (newPath.has_value())
	{
		this->setPath(newPath.value());
	}
}
