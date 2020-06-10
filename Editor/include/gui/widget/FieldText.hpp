#pragma once

#include "TemportalEnginePCH.hpp"

#include "utility/StringUtils.hpp"
#include "math/compare.h"

#include <imgui.h>

NS_GUI

template <ui32 MaxCharLength>
class FieldText
{

public:

	static bool Inline(std::string title, std::string &value, ImGuiInputTextFlags flags = ImGuiInputTextFlags_None)
	{
		auto field = gui::FieldText<32>().string(value);
		auto bChanged = field.render(title, flags);
		if (bChanged) value = field.string();
		return bChanged;
	}

	FieldText& string(std::string content)
	{
		this->mRawString.fill('\0'); // fills with end of line '\0'
		memcpy(this->mRawString.data(), content.data(), minimum(content.length(), this->mRawString.size()));
		return *this;
	}

	std::string string() const
	{
		return utility::createStringFromFixedArray(this->mRawString);
	}

	bool render(std::string title, ImGuiInputTextFlags flags = ImGuiInputTextFlags_None)
	{
		return ImGui::InputText(title.c_str(), this->mRawString.data(), this->mRawString.size(), flags);
	}

private:
	std::array<char, MaxCharLength> mRawString;

};

NS_END
