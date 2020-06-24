#pragma once

#include "TemportalEnginePCH.hpp"

#include "utility/StringUtils.hpp"

#define NS_EDITOR namespace editor {

NS_EDITOR

class Commandlet
{

public:
	virtual std::string getId() const = 0;
	virtual void initialize(utility::ArgumentMap args) = 0;
	virtual void run() = 0;

};

NS_END
