#include "utility/StringUtils.hpp"

#include <cstdarg>
#include <sstream>
#include <iostream>

bool utility::startsWith(std::string a, std::string prefix)
{
	return a.substr(0, prefix.length()) == prefix;
}

std::string utility::afterPrefix(std::string a, std::string prefix)
{
	assert(utility::startsWith(a, prefix));
	return a.substr(prefix.length());
}

std::vector<std::string> utility::split(std::string const& str, char delimeter)
{
	std::stringstream ss(str);
	std::string item;
	std::vector<std::string> splittedStrings;
	while (std::getline(ss, item, delimeter))
	{
		splittedStrings.push_back(item);
	}
	return splittedStrings;
}

utility::ArgumentMap utility::parseArguments(int argc, char *argv[])
{
	ArgumentMap args;
	for (auto i = 1; i < argc; ++i)
	{
		auto arg = std::string(argv[i]);
		std::optional<std::string> value = std::nullopt;
		auto delimiterPos = arg.find('=');
		if (delimiterPos != std::string::npos)
		{
			value = arg.substr(delimiterPos + 1);
			arg = arg.substr(0, delimiterPos);
		}
		args.insert(std::make_pair(arg, value));
	}
	return args;
}

utility::ArgumentMap utility::getArgumentsWithPrefix(ArgumentMap args, std::string prefix)
{
	auto filtered = std::unordered_map<std::string, std::optional<std::string>>();
	for (auto&[arg, val] : args)
	{
		if (!utility::startsWith(arg, prefix)) continue;
		auto argKey = utility::afterPrefix(arg, prefix);
		if (argKey.empty()) continue;
		filtered.insert(std::make_pair(argKey, val));
	}
	return filtered;
}

std::unordered_map<std::string, uSize> utility::parseArgumentInts(ArgumentMap args, std::string prefix, uSize &sum)
{
	return utility::parseArgumentsWithPrefix<uSize>(args, prefix, [&sum](std::string arg, std::optional<std::string> value) -> std::optional<uSize>
	{
		if (!value.has_value()) return std::nullopt;
		auto size = (uSize)std::stoi(value.value());
		sum += size;
		return size;
	});
}

std::vector<char const*> utility::createTemporaryStringSet(std::vector<std::string> const &strs)
{
	auto ret = std::vector<const char*>(strs.size());
	for (ui32 i = 0; i < strs.size(); ++i)
	{
		ret[i] = strs[i].c_str();
	}
	return ret;
}

std::string utility::formatStr(std::string const format, ...)
{
	va_list args;

	va_start(args, format);
	size_t len = std::vsnprintf(NULL, 0, format.c_str(), args);
	va_end(args);

	std::vector<char> formatted(len + 1);

	va_start(args, format);
	std::vsnprintf(&formatted[0], len + 1, format.c_str(), args);
	va_end(args);

	return std::string(&formatted[0]);
}

template <> std::string utility::StringParser<std::string>::parse(std::string str) { return str; }
template <> std::string utility::StringParser<std::string>::to_string(std::string const& v) { return v; }

template <> bool utility::StringParser<bool>::parse(std::string v) { return v == "true"; }
template <> std::string utility::StringParser<bool>::to_string(bool const& v) { return v ? "true" : "false"; }

template <> i8 utility::StringParser<i8>::parse(std::string v) { return (i8)std::stoi(v); }
template <> std::string utility::StringParser<i8>::to_string(i8 const& v) { return std::to_string((i32)v); }
template <> ui8 utility::StringParser<ui8>::parse(std::string v) { return (ui8)std::stoi(v); }
template <> std::string utility::StringParser<ui8>::to_string(ui8 const& v) { return std::to_string((ui32)v); }
template <> i16 utility::StringParser<i16>::parse(std::string v) { return (i16)std::stoi(v); }
template <> std::string utility::StringParser<i16>::to_string(i16 const& v) { return std::to_string((i32)v); }
template <> ui16 utility::StringParser<ui16>::parse(std::string v) { return (ui16)std::stoi(v); }
template <> std::string utility::StringParser<ui16>::to_string(ui16 const& v) { return std::to_string((ui32)v); }

template <> i32 utility::StringParser<i32>::parse(std::string str) { return std::stoi(str); }
template <> std::string utility::StringParser<i32>::to_string(i32 const& v) { return std::to_string(v); }
// there is no dedicated string-to-int for regular 32-bit unsigned ints
template <> ui32 utility::StringParser<ui32>::parse(std::string arg) { return (ui32)std::stoi(arg); }
template <> std::string utility::StringParser<ui32>::to_string(ui32 const& v) { return std::to_string(v); }
template <> i64 utility::StringParser<i64>::parse(std::string arg) { return std::stoll(arg); }
template <> std::string utility::StringParser<i64>::to_string(i64 const& v) { return std::to_string(v); }
template <> ui64 utility::StringParser<ui64>::parse(std::string arg) { return std::stoull(arg); }
template <> std::string utility::StringParser<ui64>::to_string(ui64 const& v) { return std::to_string(v); }
template <> f32 utility::StringParser<f32>::parse(std::string arg) { return std::stof(arg); }
template <> std::string utility::StringParser<f32>::to_string(f32 const& v) { return std::to_string(v); }
template <> f64 utility::StringParser<f64>::parse(std::string arg) { return std::stod(arg); }
template <> std::string utility::StringParser<f64>::to_string(f64 const& v) { return std::to_string(v); }
template <> f128 utility::StringParser<f128>::parse(std::string arg) { return std::stold(arg); }
template <> std::string utility::StringParser<f128>::to_string(f128 const& v) { return std::to_string(v); }
