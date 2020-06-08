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
	typedef std::function<bool(TItem& item)> ItemRenderer;

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

	bool render(std::string id, std::string title, bool bItemsCollapse)
	{
		assert(this->mValueRenderer);

		
		const auto bShowContent = ImGui::TreeNodeEx(
			id.c_str(), ImGuiTreeNodeFlags_::ImGuiTreeNodeFlags_FramePadding,
			"%s (%i items)", title.c_str(), this->mItems.size()
		);
		
		ImGui::SameLine();
		this->renderPostHeader();
		
		if (bShowContent)
		{
			auto iterToRemove = this->mItems.end();
			for (auto it = this->mItems.begin(); it != this->mItems.end(); ++it)
			{
				ui64 idx = it - this->mItems.begin();
				ImGui::PushID((ui32)idx);
				if (bItemsCollapse)
				{
					const auto bShowItemContent = ImGui::TreeNodeEx(
						std::to_string(idx).c_str(),
						ImGuiTreeNodeFlags_::ImGuiTreeNodeFlags_FramePadding,
						""
					);
					if (this->mKeyRenderer)
					{
						ImGui::SameLine();
						this->mKeyRenderer(*it);
					}
					ImGui::SameLine();
					if (ImGui::Button("Remove"))
					{
						iterToRemove = it;
					}
					if (bShowItemContent)
					{
						this->mValueRenderer(*it);
						ImGui::TreePop();
					}
				}
				else
				{
					if (this->mKeyRenderer)
					{
						this->mKeyRenderer(*it);
						ImGui::SameLine();
						if (ImGui::Button("Remove"))
						{
							iterToRemove = it;
						}
					}
					this->mValueRenderer(*it);
					if (!this->mKeyRenderer)
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

			if (iterToRemove != this->mItems.end())
			{
				this->mItems.erase(iterToRemove);
			}
		}
		return false;
	}

private:
	std::vector<TItem> mItems;
	ItemRenderer mKeyRenderer;
	ItemRenderer mValueRenderer;

	void renderPostHeader()
	{
		if (ImGui::Button("Add"))
		{
			this->mItems.push_back(TItem());
		}
	}

};

NS_END
