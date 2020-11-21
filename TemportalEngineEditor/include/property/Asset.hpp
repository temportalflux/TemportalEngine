#pragma once

#include "property/Base.hpp"

#include "Editor.hpp"
#include "asset/AssetPath.hpp"
#include "asset/TypedAssetPath.hpp"
#include "asset/AssetManager.hpp"

#include <imgui.h>

NS_PROPERTIES

template <class TAsset>
DECLARE_PROPERTY_EDITOR(asset::TypedAssetPath<TAsset>)
{
	PropertyResult result;
	result.bChangedValue = false;
	auto const assetType = asset::TypedAssetPath<TAsset>::StaticType();
	bool bShowOptions = ImGui::BeginCombo(id, value.path().filename().c_str(), ImGuiComboFlags_None);
	result.bIsHovered = ImGui::IsItemHovered();
	if (bShowOptions)
	{
		auto const assetPathList = asset::AssetManager::get()->getAssetList(assetType);
		for (asset::AssetPath const& option : assetPathList)
		{
			bool bIsSelected = option == value.path();
			ImGui::PushID(option.toString().c_str());
			if (ImGui::Selectable(option.filename().c_str(), bIsSelected))
			{
				value.path() = option;
				result.bChangedValue = true;
			}
			if (bIsSelected) ImGui::SetItemDefaultFocus();
			ImGui::PopID();
		}
		ImGui::EndCombo();
	}
	if (ImGui::BeginDragDropTarget())
	{
		ImGuiPayload const* activePayload = ImGui::GetDragDropPayload();
		bool bCanAcceptPayload = activePayload->IsDataType("_ASSETPATH") && (((asset::AssetPath*)(activePayload->Data))->type() == assetType);
		if (bCanAcceptPayload)
		{
			if (ImGuiPayload const *payload = ImGui::AcceptDragDropPayload("_ASSETPATH", ImGuiDragDropFlags_None))
			{
				value.path() = *((asset::AssetPath*)(payload->Data));
				result.bChangedValue = true;
			}
		}
		ImGui::EndDragDropTarget();
	}
	return result;
}

template <typename TAsset>
bool renderProperty(std::string id, asset::TypedAssetPath<TAsset> &value, asset::TypedAssetPath<TAsset> const& defaultValue)
{
	properties::contextMenuEntries.push_back({
		"Edit Asset", [value]()
		{
			Editor::openAssetEditorAt(value.path());
			return false;
		}
	});
	bool bChanged = properties::renderProperty_internal<asset::TypedAssetPath<TAsset>>(id, value, defaultValue);
	properties::contextMenuEntries.pop_back();
	return bChanged;
}

NS_END
