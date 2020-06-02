#pragma once

#include "TemportalEnginePCH.hpp"

#include "utility/StringUtils.hpp"

#define NS_EDITOR namespace editor {

NS_EDITOR

class Commandlet
{

public:

	virtual std::string getId() const
	{
		return "";
	}

	virtual void run(utility::ArgumentMap args) {}

};

NS_END
