#pragma once

#include "TemportalEnginePCH.hpp"

NS_GUI

// T = f32/i32/ui32
// Count = amount of numbers
template <typename T, ui8 Count>
class FieldNumber
{

public:

	constexpr static ImGuiDataType_ GuiDataType()
	{
#define GuiDataTypeMatch(ValueType, EnumType) constexpr (std::is_same<T, ValueType>::value) return EnumType
		/**/ if GuiDataTypeMatch(ui32, ImGuiDataType_::ImGuiDataType_U32);
		else if GuiDataTypeMatch(i32, ImGuiDataType_::ImGuiDataType_S32);
		else if GuiDataTypeMatch(ui16, ImGuiDataType_::ImGuiDataType_U16);
		else if GuiDataTypeMatch(i16, ImGuiDataType_::ImGuiDataType_S16);
		else if GuiDataTypeMatch(ui8, ImGuiDataType_::ImGuiDataType_U8);
		else if GuiDataTypeMatch(i8, ImGuiDataType_::ImGuiDataType_S8);
		else if GuiDataTypeMatch(ui64, ImGuiDataType_::ImGuiDataType_U64);
		else if GuiDataTypeMatch(i64, ImGuiDataType_::ImGuiDataType_S64);
		else if GuiDataTypeMatch(f32, ImGuiDataType_::ImGuiDataType_Float);
		else if GuiDataTypeMatch(f64, ImGuiDataType_::ImGuiDataType_Double);
#undef GuiDataTypeMatch
	}

	FieldNumber<T, Count>& value(std::array<T, Count> all)
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
			label.c_str(), GuiDataType(),
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

NS_END
