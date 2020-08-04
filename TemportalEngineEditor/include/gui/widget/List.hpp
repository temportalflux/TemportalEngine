#pragma once

#include "TemportalEnginePCH.hpp"

#include <type_traits>
#include <imgui.h>

NS_GUI

// If representing a map, TItem will be an std::pair<Key, Value>
// otherwise it could be any value type
template <typename TItem>
class List
{

	template <typename TKey, typename TValue>
	using EnableIfMap = typename std::enable_if<std::is_same<TItem, std::pair<TKey, TValue>>::value>::type;

public:
	typedef std::function<bool(uIndex const& idx, TItem& item)> ItemRenderer;

	List<TItem>& value(std::vector<TItem> const &items)
	{
		this->mItems.assign(items.cbegin(), items.cend());
		return *this;
	}

	template <typename TKey, typename TValue>
	List<TItem>& value(std::map<TKey, TValue> const &items, EnableIfMap<TKey, TValue>* = nullptr)
	{
		this->mItems.assign(items.cbegin(), items.cend());
		return *this;
	}

	List<TItem>& setKeyRenderer(ItemRenderer render)
	{
		this->mKeyRenderer = render;
		return *this;
	}

	List<TItem>& setValueRenderer(ItemRenderer render)
	{
		this->mValueRenderer = render;
		return *this;
	}

	static bool Inline(
		char const* id, char const* title, bool bItemsCollapse,
		std::vector<TItem> &items,
		ItemRenderer renderValue, std::optional<ItemRenderer> renderKey = std::nullopt
	)
	{
		bool bHasChanged = false;

		const auto bShowContent = ImGui::TreeNodeEx(
			id, ImGuiTreeNodeFlags_::ImGuiTreeNodeFlags_FramePadding,
			"%s (%i items)", title, items.size()
		);

		ImGui::SameLine();
		if (ImGui::Button("Add"))
		{
			items.push_back(TItem());
			bHasChanged = true;
		}

		if (bShowContent)
		{
			auto iterToRemove = items.end();
			for (auto it = items.begin(); it != items.end(); ++it)
			{
				uIndex idx = it - items.begin();
				ImGui::PushID((ui32)idx);
				if (bItemsCollapse)
				{
					const auto bShowItemContent = ImGui::TreeNodeEx(
						std::to_string(idx).c_str(),
						ImGuiTreeNodeFlags_::ImGuiTreeNodeFlags_FramePadding,
						""
					);
					if (renderKey)
					{
						ImGui::SameLine();
						(*renderKey)(idx, *it);
					}
					ImGui::SameLine();
					if (ImGui::Button("Remove"))
					{
						iterToRemove = it;
					}
					if (bShowItemContent)
					{
						if (renderValue(idx, *it)) bHasChanged = true;
						ImGui::TreePop();
					}
				}
				else
				{
					if (renderKey)
					{
						(*renderKey)(idx, *it);
						ImGui::SameLine();
						if (ImGui::Button("Remove"))
						{
							iterToRemove = it;
						}
					}
					if (renderValue(idx, *it)) bHasChanged = true;
					if (!renderKey)
					{
						ImGui::SameLine();
						if (ImGui::Button("Remove"))
						{
							iterToRemove = it;
						}
					}
				}
				ImGui::PopID();
			}
			ImGui::TreePop();

			if (iterToRemove != items.end())
			{
				items.erase(iterToRemove);
				bHasChanged = true;
			}
		}
		return bHasChanged;
	}

	bool render(std::string id, std::string title, bool bItemsCollapse)
	{
		return Inline(id.c_str(), title.c_str(), bItemsCollapse, this->mItems, this->mValueRenderer, this->mKeyRenderer);
	}

private:
	std::vector<TItem> mItems;
	ItemRenderer mKeyRenderer;
	ItemRenderer mValueRenderer;

};

NS_END
