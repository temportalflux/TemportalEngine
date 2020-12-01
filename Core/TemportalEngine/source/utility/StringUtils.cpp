#include "utility/StringUtils.hpp"

#include <cstdarg>

bool utility::startsWith(std::string a, std::string prefix)
{
	return a.substr(0, prefix.length()) == prefix;
}

std::string utility::afterPrefix(std::string a, std::string prefix)
{
	assert(utility::startsWith(a, prefix));
	return a.substr(prefix.length());
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
