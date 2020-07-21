#include "gui/widget/FileDirectoryTree.hpp"

#include <imgui.h>

using namespace gui;

void FileDirectoryTree::setRootDirectory(std::filesystem::path const &path)
{
	this->mRootDirPath = path;
}

void FileDirectoryTree::render()
{
	if (this->mRootDirPath.empty()) return;

	ImGui::Text(this->mRootDirPath.string().c_str());

	if (!std::filesystem::exists(this->mRootDirPath))
	{
		ImGui::Text("Missing directory, cannot render contents");
		return;
	}
	else if (std::filesystem::is_empty(this->mRootDirPath))
	{
		ImGui::Text("Directory is empty");
		return;
	}

	this->mPathSelectedDuringRender = std::nullopt;
	this->renderSubDirectoryContents(this->mRootDirPath);
}

std::optional<std::filesystem::path> const& FileDirectoryTree::getLastSelectedPath() const
{
	return this->mPathSelectedDuringRender;
}

void FileDirectoryTree::renderSubDirectory(std::filesystem::path const &path)
{
	bool bShowContents = ImGui::TreeNodeEx(path.stem().string().c_str(), ImGuiTreeNodeFlags_OpenOnArrow);
	if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
	{
		this->mPathSelectedDuringRender = path;
	}
	if (bShowContents)
	{
		this->renderSubDirectoryContents(path);
		ImGui::TreePop();
	}
}

void FileDirectoryTree::renderSubDirectoryContents(std::filesystem::path const &path)
{
	for (const auto& entry : std::filesystem::directory_iterator(path))
	{
		if (!entry.is_directory()) continue;
		this->renderSubDirectory(entry.path());
	}
}
