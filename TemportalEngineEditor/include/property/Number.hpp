#pragma once

#include "property/Base.hpp"

#include <imgui.h>

NS_PROPERTIES

template <typename TNumber>
struct NumberSettings
{
	std::optional<TNumber> step;
	std::optional<TNumber> stepFast;
};

template <typename TNumber>
struct PropertyNumber : public PropertyMinimal<TNumber>, public NumberSettings<TNumber>
{
	PropertyNumber() : PropertyMinimal<TNumber>() {}
	PropertyNumber(
		TNumber initial, TNumber value,
		std::optional<TNumber> step = std::nullopt, std::optional<TNumber> stepFast = std::nullopt
	) : PropertyMinimal<TNumber>(initial, value)
	{
		this->step = step;
		this->stepFast = stepFast;
	}

};

#define DECLARE_PROPERTY_EDITOR_NUM(TYPE_NUM) DECLARE_PROPERTY_EDITOR(PropertyNumber<TYPE_NUM>)
DECLARE_PROPERTY_EDITOR_NUM(i8);
DECLARE_PROPERTY_EDITOR_NUM(i16);
DECLARE_PROPERTY_EDITOR_NUM(i32);
DECLARE_PROPERTY_EDITOR_NUM(i64);
DECLARE_PROPERTY_EDITOR_NUM(ui8);
DECLARE_PROPERTY_EDITOR_NUM(ui16);
DECLARE_PROPERTY_EDITOR_NUM(ui32);
DECLARE_PROPERTY_EDITOR_NUM(ui64);
DECLARE_PROPERTY_EDITOR_NUM(f32);
DECLARE_PROPERTY_EDITOR_NUM(f64);
#undef DECLARE_PROPERTY_EDITOR_NUM

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
bool renderNumbers(const char* id, T const* initial, T* data, ui32 count, NumberSettings<T> const& prop)
{
	return ImGui::InputScalarN(
		id, GuiDataType<T>(),
		data, count,
		(void*)(prop.step.has_value() ? &prop.step.value() : nullptr),
		(void*)(prop.stepFast.has_value() ? &prop.stepFast.value() : nullptr),
		nullptr, ImGuiInputTextFlags_None
	);
}

NS_END
