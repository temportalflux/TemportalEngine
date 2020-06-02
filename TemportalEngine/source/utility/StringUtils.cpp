#include "utility/StringUtils.hpp"

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

std::unordered_map<std::string, ui64> utility::parseArgumentInts(ArgumentMap args, std::string prefix, ui64 &sum)
{
	return utility::parseArgumentsWithPrefix<ui64>(args, prefix, [&sum](std::string arg, std::optional<std::string> value) -> std::optional<ui64>
	{
		if (!value.has_value()) return std::nullopt;
		auto size = (ui64)std::stoi(value.value());
		sum += size;
		return size;
	});
}

