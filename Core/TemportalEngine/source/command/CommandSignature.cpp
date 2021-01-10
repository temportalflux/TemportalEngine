#include "command/CommandSignature.hpp"

using namespace command;

Signature::Signature(std::string const& id) : mId(id) {}

Signature& Signature::bind(TCallback callback)
{
	this->mCallback = callback;
	return *this;
}

std::string const& Signature::id() const { return this->mId; }

std::optional<std::string> Signature::parse(TArgList const& args)
{
	auto iterArgValue = args.begin();
	auto iterArgType = this->mArgTypes.begin();
	while (iterArgType != this->mArgTypes.end())
	{
		if (iterArgValue == args.end())
		{
			return "Missing arguments";
		}

		try
		{
			std::any anyArg = (*iterArgType)(*iterArgValue);
			this->mArgs.push_back(anyArg);
			iterArgValue++;
			iterArgType++;
		}
		catch (std::exception e)
		{
			return utility::formatStr("Could not parse %s", iterArgValue->c_str());
		}
	}
	return std::nullopt;
}

void Signature::execute()
{
	this->mCallback(*this);
}
