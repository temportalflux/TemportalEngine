#pragma once

#include "settings/Settings.hpp"

NS_GAME

class UserSettings
	: public Settings
{

public:
	UserSettings()
		: Settings("userSettings.json")
		, mName("unspecified")
	{
	}

	UserSettings& setName(std::string const& name) { this->mName = name; return *this; }
	std::string const& name() const { return this->mName; }

private:

	// The name of the user. This can eventually pull from something like Steam integration.
	std::string mName;

protected:

	void save(cereal::JSONOutputArchive &archive) const override
	{
		archive(cereal::make_nvp("name", this->mName));
	}

	void load(cereal::JSONInputArchive &archive) override
	{
		archive(cereal::make_nvp("name", this->mName));
	}

};

NS_END
