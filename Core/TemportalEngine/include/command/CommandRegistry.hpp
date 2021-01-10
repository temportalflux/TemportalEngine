#pragma once

#include "TemportalEnginePCH.hpp"

#include "command/CommandSignature.hpp"

NS_COMMAND

class Registry
{

public:

	void add(command::Signature const& signature);
	
	std::optional<std::vector<std::string>> execute(std::vector<std::string> args);

private:
	std::multimap<std::string, command::Signature> mSignatures;

};

NS_END
