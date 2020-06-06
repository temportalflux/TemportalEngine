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
	typedef std::function<i32(T)> ToIdFunctor;

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

	Combo<T>& setCallbacks(ToStringFunctor to_string, ToIdFunctor to_id)
	{
		this->mfToString = to_string;
		this->mfToId = to_id;
		return *this;
	}

	bool render(std::string title)
	{
		bool bHasChanged = false;
		if (ImGui::BeginCombo(title.c_str(), this->mfToString(this->mSelected).c_str(), ImGuiComboFlags_None))
		{
			for (auto& option : this->mOptions)
			{
				bool bIsSelected = option == this->mSelected;
				auto optionString = this->mfToString(option);
				ImGui::PushID(this->mfToId(option));
				if (ImGui::Selectable(optionString.c_str(), bIsSelected))
				{
					this->mSelected = option;
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
	ToIdFunctor mfToId;

};

NS_END
