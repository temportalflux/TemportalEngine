#pragma once

#include "settings/Settings.hpp"

NS_GAME

class ServerSettings
	: public Settings
{

public:
	ServerSettings()
		: Settings("serverSettings.json")
		, mPort(25565)
	{
	}

	ui16 port() const { return this->mPort; }

private:

	ui16 mPort;

protected:

	void save(cereal::JSONOutputArchive &archive) const override
	{
		archive(cereal::make_nvp("port", this->mPort));
	}

	void load(cereal::JSONInputArchive &archive) override
	{
		archive(cereal::make_nvp("port", this->mPort));
	}

};

NS_END
