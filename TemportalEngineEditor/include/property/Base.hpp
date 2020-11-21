#pragma once

#include "TemportalEnginePCH.hpp"

#define NS_PROPERTIES namespace properties {

NS_PROPERTIES

struct ContextMenuEntry
{
	std::string label;
	std::function<bool()> action;
};

static std::vector<ContextMenuEntry> contextMenuEntries = std::vector<ContextMenuEntry>();

template <typename T>
struct PropertyMinimal
{
	T initial;
	T value;
	bool bIsMultiline;

	PropertyMinimal() : bIsMultiline(false) {}
	PropertyMinimal(T const initial, T value) : initial(initial), value(value), bIsMultiline(false) {}
	PropertyMinimal<T>& operator=(PropertyMinimal<T> const& other)
	{
		this->initial = other.initial;
		this->value = other.value;
		this->bIsMultiline = other.bIsMultiline;
		return *this;
	}
};

struct PropertyResult
{
	bool bChangedValue;
	bool bIsHovered;
	static PropertyResult oneLine(bool bChangedValue);
	static PropertyResult group(char const* id, std::function<void(bool &bChangedAny)> renderContent);
};

#define DECLARE_PROPERTY_EDITOR(VAL_TYPE) PropertyResult renderPropertyEditor(const char* id, VAL_TYPE &value, VAL_TYPE const& defaultValue)
#define DEFINE_PROPERTY_EDITOR(VAL_TYPE) PropertyResult properties::renderPropertyEditor(const char* id, VAL_TYPE &value, VAL_TYPE const& defaultValue)

template <typename TValue>
bool renderProperty(std::string id, TValue &value, TValue const& defaultValue);

NS_END
