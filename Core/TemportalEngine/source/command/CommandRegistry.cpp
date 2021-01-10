#include "command/CommandRegistry.hpp"

using namespace command;

void Registry::add(command::Signature const& signature)
{
	this->mSignatures.insert(std::make_pair(signature.id(), signature));
}

std::optional<std::vector<std::string>> Registry::execute(std::vector<std::string> args)
{
	assert(args.size() > 0);
	auto id = *args.begin();
	args.erase(args.begin());
	
	auto signaturesWithId = this->mSignatures.equal_range(id);
	if (signaturesWithId.first == signaturesWithId.second)
	{
		return std::vector<std::string> { "Failed to recognize command" };
	}

	auto parsingErrors = std::vector<std::string>();
	for (auto iter = signaturesWithId.first; iter != signaturesWithId.second; ++iter)
	{
		auto signatureCopy = iter->second;
		if (auto parsingError = signatureCopy.parse(args))
		{
			parsingErrors.push_back(parsingError.value());
		}
		else
		{
			signatureCopy.execute();
			return std::nullopt;
		}
	}
	return parsingErrors;
}
