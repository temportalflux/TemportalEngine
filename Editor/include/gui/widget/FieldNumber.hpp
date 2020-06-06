#pragma once

#include "TemportalEnginePCH.hpp"

NS_GUI

// T = f32/i32/ui32
// Count = amount of numbers
template <
	typename T, ui8 Count,
	ImGuiDataType_ TGuiType
>
class FieldNumber
{

public:

	FieldNumber<T, Count, TGuiType>& value(std::array<T, Count> all)
	{
		this->mRaw = all;
		return *this;
	}

	T operator[](ui8 idx) const
	{
		return this->mRaw[idx];
	}

	T& operator[](ui8 idx)
	{
		return this->mRaw[idx];
	}

	bool render(std::string label, T step=0, T stepFast=0, char const* format=nullptr)
	{
		return ImGui::InputScalarN(
			label.c_str(), TGuiType,
			this->mRaw.data(), Count,
			(void*)(step > 0 ? &step : nullptr),
			(void*)(stepFast > 0 ? &stepFast : nullptr),
			format,
			ImGuiInputTextFlags_None
		);
	}

private:
	std::array<T, Count> mRaw;

};

template <ui8 Count>
using Field_ui32 = FieldNumber<ui32, Count, ImGuiDataType_::ImGuiDataType_U32>;

NS_END
