#include "gui/AssetReferenceViewer.hpp"

#include "Engine.hpp"
#include "utility/StringUtils.hpp"
#include "asset/AssetManager.hpp"

#include <imgui.h>

using namespace gui;

AssetReferenceViewer::AssetReferenceViewer(std::string title) : IGui(title)
{
}

AssetReferenceViewer::~AssetReferenceViewer()
{
}

void AssetReferenceViewer::setAssetFilePath(std::filesystem::path const& absolutePath)
{
	this->mAbsolutePath = absolutePath;
}

i32 AssetReferenceViewer::getFlags() const
{
	return ImGuiWindowFlags_None;
}

void AssetReferenceViewer::renderView()
{
	auto manager = asset::AssetManager::get();
	ImGui::BeginChild("scroll-area", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);

	ImGui::Columns(2);
	if (ImGui::CollapsingHeader("Referenced By", ImGuiTreeNodeFlags_DefaultOpen))
	{
		auto iterRange = manager->getAssetPathsWhichReference(this->mAbsolutePath);
		for (auto iter = iterRange.first; iter != iterRange.second; ++iter)
		{
			std::string display = iter->second.filename() + " (" + iter->second.path().parent_path().string() + ")";
			ImGui::Text(display.c_str());
		}
	}
	ImGui::NextColumn();
	if (ImGui::CollapsingHeader("References", ImGuiTreeNodeFlags_DefaultOpen))
	{
		auto iterRange = manager->getAssetPathsReferencedBy(this->mAbsolutePath);
		for (auto iter = iterRange.first; iter != iterRange.second; ++iter)
		{
			std::string display = iter->second.filename() + " (" + iter->second.path().parent_path().string() + ")";
			ImGui::Text(display.c_str());
		}
	}

	ImGui::EndChild();
}
