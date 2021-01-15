#pragma once

#include "TemportalEnginePCH.hpp"

#include "Engine.hpp"
#include "utility/StringUtils.hpp"

#include <typeinfo>
#include <typeindex>
#include <any>

#define CMD_SIGNATURE(ID, CLASS_TYPE, INST, MEMBER_FUNC) command::Signature(ID).bind(std::bind(&CLASS_TYPE::MEMBER_FUNC, INST, std::placeholders::_1))
#define CMD_SIGNATURE_0(ID, CLASS_TYPE, INST, MEMBER_FUNC) command::Signature(ID).bind(std::bind(&CLASS_TYPE::MEMBER_FUNC, INST))
#define ADD_CMD(CMD) engine::Engine::Get()->commands()->add(CMD)

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
