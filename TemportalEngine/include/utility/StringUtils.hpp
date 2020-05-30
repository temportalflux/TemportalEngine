#pragma once

#include <string>
#include <array>

namespace utility {

	template <size_t TSize>
	std::string createStringFromFixedArray(std::array<char, TSize> fixed)
	{
		auto str = std::string(fixed.begin(), fixed.end());
		str.erase(std::find_if(str.begin(), str.end(), [&](int c) {
			return c == '\0';
		}), str.end());
		return str;
	}

}