#pragma once

#include "commandlet/Commandlet.hpp"

#include <filesystem>

NS_EDITOR

class CommandletBuildAssets : public Commandlet
{

public:
	std::string getId() const override { return "buildAssets"; }
	void initialize(utility::ArgumentMap args) override;
	void run() override;

private:
	std::filesystem::path mPathProjectAsset;
	std::filesystem::path mPathOutputDir;

};

NS_END
