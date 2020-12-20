#pragma once

#include "TemportalEnginePCH.hpp"

template <typename TKey, typename TValue>
void mapEraseIf(std::map<TKey, TValue> &map, std::function<bool(TKey const& key, TValue const& value)> predicate)
{
	for (auto iter = map.begin(); iter != map.end();)
	{
		if (predicate(iter->first, iter->second)) iter = map.erase(iter);
		else ++iter;
	}
}
