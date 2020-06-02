#pragma once

#include "commandlet/Commandlet.hpp"

NS_EDITOR

class CommandletBuildAssets : public Commandlet
{

public:
	std::string getId() const override { return "buildAssets"; }
	void run(utility::ArgumentMap args) override;

};

NS_END
