#pragma once

#include "property/Base.hpp"

#include <imgui.h>

NS_PROPERTIES

DECLARE_PROPERTY_EDITOR(i8);
DECLARE_PROPERTY_EDITOR(i16);
DECLARE_PROPERTY_EDITOR(i32);
DECLARE_PROPERTY_EDITOR(i64);
DECLARE_PROPERTY_EDITOR(ui8);
DECLARE_PROPERTY_EDITOR(ui16);
DECLARE_PROPERTY_EDITOR(ui32);
DECLARE_PROPERTY_EDITOR(ui64);
DECLARE_PROPERTY_EDITOR(f32);
DECLARE_PROPERTY_EDITOR(f64);

template <typename T>
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

template <typename T>
PropertyResult renderNumbers(const char* id, T const* initial, T* data, ui32 count)
{
	return PropertyResult::oneLine(ImGui::InputScalarN(
		id, GuiDataType<T>(),
		data, count,
		nullptr, nullptr, // TODO: Figure out how to push properties into the stack
		nullptr, ImGuiInputTextFlags_None
	));
}

NS_END
