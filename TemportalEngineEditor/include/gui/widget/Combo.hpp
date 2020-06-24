#pragma once

#include "TemportalEnginePCH.hpp"

#include <imgui.h>

NS_GUI

template <typename T>
class Combo
{

public:
	typedef std::vector<T> OptionsList;
	typedef std::function<std::string(T)> ToStringFunctor;
	typedef std::function<void(T)> PushIdFunctor;

	Combo<T>& setOptions(OptionsList options)
	{
		this->mOptions = options;
		return *this;
	}

	Combo<T>& value(T option)
	{
		this->mSelected = option;
		return *this;
	}

	T value() const
	{
		return this->mSelected;
	}

	Combo<T>& setCallbacks(ToStringFunctor to_string, PushIdFunctor to_id)
	{
		this->mfToString = to_string;
		this->mfToId = to_id;
		return *this;
	}

	bool render(std::string title)
	{
		return Inline(title.c_str(), this->mOptions, this->mSelected, this->mfToString, this->mfToId);
	}

	static bool Inline(const char* titleId, OptionsList const &options, T &selected, ToStringFunctor toString, PushIdFunctor pushId, ImGuiComboFlags flags = ImGuiComboFlags_None)
	{
		bool bHasChanged = false;
		if (ImGui::BeginCombo(titleId, toString(selected).c_str(), flags))
		{
			for (const auto& option : options)
			{
				bool bIsSelected = option == selected;
				auto optionString = toString(option);
				pushId(option);
				if (ImGui::Selectable(optionString.c_str(), bIsSelected))
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

private:
	OptionsList mOptions;
	T mSelected;
	ToStringFunctor mfToString;
	PushIdFunctor mfToId;

};

NS_END
