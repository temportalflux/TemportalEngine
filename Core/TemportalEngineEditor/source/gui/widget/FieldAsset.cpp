#include "gui/widget/FieldAsset.hpp"

#include "Editor.hpp"
#include "asset/AssetManager.hpp"
#include "asset/AssetPath.hpp"

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
	return FieldAsset::Inline(titleId, selected, this->mAssetPaths, this->mTypeFilter);
}

bool FieldAsset::Inline(std::string titleId, asset::AssetPath &selected, std::vector<asset::AssetPath> const& options, std::optional<std::string> assetType)
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
	if (ImGui::BeginDragDropTarget())
	{
		ImGuiPayload const* activePayload = ImGui::GetDragDropPayload();
		bool bCanAcceptPayload = activePayload->IsDataType("ASSETPATH") && (!assetType || (((asset::AssetPath*)(activePayload->Data))->type() == *assetType));
		if (bCanAcceptPayload)
		{
			if (ImGuiPayload const *payload = ImGui::AcceptDragDropPayload("ASSETPATH", ImGuiDragDropFlags_None))
			{
				selected = *((asset::AssetPath*)(payload->Data));
				bHasChanged = true;
			}
		}
		ImGui::EndDragDropTarget();
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
