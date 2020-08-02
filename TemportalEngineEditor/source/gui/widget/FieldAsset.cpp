#include "gui/widget/FieldAsset.hpp"

#include "Editor.hpp"
#include "asset/AssetManager.hpp"

#include <imgui.h>

using namespace gui;

void FieldAsset::updateAssetList(std::optional<asset::AssetType> filter)
{
	if (filter == this->mTypeFilter) return;
	this->mTypeFilter = filter;
	if (!filter)
	{
		this->mAssetPaths = asset::AssetManager::get()->getAssetList();
	}
	else
	{
		this->mAssetPaths = asset::AssetManager::get()->getAssetList(*filter);
	}
}

bool FieldAsset::render(char const* id, std::string title, asset::AssetPath &selected)
{
	std::string titleId = this->mTypeFilter ? (title + "(" + *this->mTypeFilter + ")###" + id) : (title + "###" + id);
	return FieldAsset::Inline(titleId, selected, this->mAssetPaths);
}

bool FieldAsset::Inline(std::string titleId, asset::AssetPath &selected, std::vector<asset::AssetPath> const& options)
{
	bool bHasChanged = false;
	if (ImGui::BeginCombo(titleId.c_str(), selected.filename().c_str(), ImGuiComboFlags_None))
	{
		for (const auto& option : options)
		{
			bool bIsSelected = option == selected;
			ImGui::PushID(option.toString().c_str());
			if (ImGui::Selectable(option.filename().c_str(), bIsSelected))
			{
				selected = option;
				bHasChanged = true;
			}
			if (bIsSelected) ImGui::SetItemDefaultFocus();
			ImGui::PopID();
		}
		ImGui::EndCombo();
	}
	if (ImGui::BeginPopupContextItem(selected.pathStr().c_str()))
	{
		if (ImGui::Selectable("Edit"))
		{
			std::filesystem::path jsonAssetPath = std::filesystem::absolute(Editor::EDITOR->getProject()->getAbsoluteDirectoryPath() / selected.pathStr());
			Editor::EDITOR->openAssetEditor(asset::readAssetFromDisk(jsonAssetPath, asset::EAssetSerialization::Json));
		}
		ImGui::EndPopup();
	}
	return bHasChanged;
}
