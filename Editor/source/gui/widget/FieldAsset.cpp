#include "gui/widget/FieldAsset.hpp"

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
	bool bHasChanged = false;
	std::string titleId = this->mTypeFilter ? (title + "(" + *this->mTypeFilter + ")###" + id) : (title + "###" + id);
	if (ImGui::BeginCombo(titleId.c_str(), selected.filename().c_str(), ImGuiComboFlags_None))
	{
		for (const auto& option : this->mAssetPaths)
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
	return bHasChanged;
}
