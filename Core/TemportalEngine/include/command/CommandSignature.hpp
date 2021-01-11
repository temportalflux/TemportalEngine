#pragma once

#include "TemportalEnginePCH.hpp"

#include "utility/StringUtils.hpp"

#include <typeinfo>
#include <typeindex>
#include <any>

NS_COMMAND

class Signature
{

public:
	using TCallback = std::function<void(Signature const&)>;
	using TArgList = std::vector<std::string>;

	Signature(std::string const& id);

	bool isBound() const;
	Signature& bind(TCallback callback);
	
	template <typename T>
	Signature& pushArgType()
	{
		this->mArgTypes.push_back(&utility::StringParser<T>::parse);
		return *this;
	}

	std::string const& id() const;
	std::optional<std::string> parse(TArgList const& args);
	void execute();

	template <typename T>
	T get(uIndex idx) const { return std::any_cast<T>(this->mArgs[idx]); }

private:
	std::string mId;
	TCallback mCallback;
	std::vector<std::function<std::any(std::string)>> mArgTypes;

	// per execution
	std::vector<std::any> mArgs;

};

NS_END
