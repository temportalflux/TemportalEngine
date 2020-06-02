#ifndef STRING_UTILS_H
#define STRING_UTILS_H

#include "types/integer.h"

#include <string>
#include <array>
#include <optional>
#include <cassert>
#include <unordered_map>
#include <functional>

namespace utility
{

	typedef std::unordered_map<std::string, std::optional<std::string>> ArgumentMap;

	template <size_t TSize>
	std::string createStringFromFixedArray(std::array<char, TSize> fixed)
	{
		auto str = std::string(fixed.begin(), fixed.end());
		str.erase(std::find_if(str.begin(), str.end(), [&](int c) {
			return c == '\0';
		}), str.end());
		return str;
	}

	bool startsWith(std::string a, std::string prefix);

	std::string afterPrefix(std::string a, std::string prefix);

	ArgumentMap parseArguments(int argc, char *argv[]);

	ArgumentMap getArgumentsWithPrefix(ArgumentMap args, std::string prefix);

	template <typename TValue>
	std::unordered_map<std::string, TValue> parseArgumentsWithPrefix(
		ArgumentMap args,
		std::string prefix,
		std::function<std::optional<TValue>(std::string arg, std::optional<std::string> val)> parse
	)
	{
		auto parsedArgs = std::unordered_map<std::string, TValue>();
		for (auto&[arg, val] : args)
		{
			if (!utility::startsWith(arg, prefix)) continue;
			auto argKey = utility::afterPrefix(arg, prefix);
			if (argKey.empty()) continue;
			auto parsedValue = parse(arg, val);
			if (parsedValue.has_value())
			{
				parsedArgs.insert(std::make_pair(
					argKey,
					parsedValue.value()
				));
			}
		}
		return parsedArgs;
	}

	std::unordered_map<std::string, ui64> parseArgumentInts(ArgumentMap args, std::string prefix, ui64 &sum);

}

#endif STRING_UTILS_H