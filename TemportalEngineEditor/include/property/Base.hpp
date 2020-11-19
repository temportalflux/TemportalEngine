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

#define DECLARE_PROPERTY_EDITOR(PROPERTY_TYPE) bool renderPropertyEditor(const char* id, PROPERTY_TYPE &prop)

template <typename TProperty>
bool renderProperty(std::string id, TProperty &prop);

NS_END
