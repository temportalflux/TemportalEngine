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
		this->setPath(project->getAbsoluteDirectoryPath() / "assets");
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

	this->renderBreadcrumbs();

	ImGui::Separator();

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
		if (ImGui::IsItemClicked())
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
			ImGui::Text("/");
			ImGui::SameLine();
		}
	}
	if (newPath.has_value())
	{
		this->setPath(newPath.value());
	}
}
